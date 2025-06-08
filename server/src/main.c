#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "filespec.h"
#include "poll.h"
#include "rw.h"
#include "socket.h"
#include "ssort.h"
#include "util.h"

#define NUM_ARGS 3
#define ARG_PROGRAM_NAME 0
#define ARG_FILESPEC_PATH 1
#define ARG_PORT 2
#define INIT_SORTED_SIZE 32
#define RESIZE_FACTOR 2
#define LINE_SIZE 256   // Max line length

// Attribution [3]
//#define DEBUG // uncomment this line to enable debugging

#ifdef DEBUG
/* When debugging is enabled, these form aliases to useful functions */
#define dbg_printf(...) printf(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated for these */
#define dbg_printf(...)
#endif


int main(int argc, char **argv) {
    enum ret_type err;

    if (argc < NUM_ARGS) {
        ret(ERR_NOT_ENOUGH_ARGS, "Please run: %s [filespec] [port]", argv[ARG_PROGRAM_NAME]);
    }

    // Open the filespec
    FILE *filespec = NULL;
    char *pathname = argv[ARG_FILESPEC_PATH];

    if ((err = fspec_open(&filespec, pathname)) != SUCCESS) {
       ret(err, "Unable to open filespec"); 
    }   

    // Get the output file
    FILE *output = NULL;

    if ((err = fspec_openoutput(filespec, &output)) != SUCCESS) {
        ret(err, "Unable to open output file");
    }

    // Get the file fragments
    // This is an array of integers (file descriptors)
    // that our filespec library allocates for us.
    int *fragments = NULL;
    int fragment_count = 0;

    if ((err = fspec_openfragments(filespec, &fragments, &fragment_count))) {
        ret(err, "Unable to open fragment files")
    }

    // Once the fragments are open, establish a socket connection.
    int sockfd;
    int port_num = atol(argv[ARG_PORT]);

    if((err = socket_open(&sockfd, port_num) != SUCCESS)) {
        ret(err, "Unable to establish a socket connection");
    }

    // Create a client socket for accepting later
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(struct sockaddr_in);

    // Open a network polling instance (for socket + clients)
    int epfd;
    if ((err = poll_open(&epfd)) != SUCCESS) {
        ret(err, "Unable to open polling instance");
    }

    // Create epoll data structures
    struct poll_event revents[MAX_EVENT_COUNT];
    unsigned int event_count;

    // Add socket to polling instance
    struct poll_data socket_data;

    socket_data.fd = sockfd;
    socket_data.type = SOCKET;

    if ((err = poll_add(epfd, sockfd, &socket_data, POLL_DATA)) != SUCCESS) {
        ret(err, "Unable to add socket to polling instance");
    }

    // Allocate client polling data
    // Do not add them to the polling instance yet, since they don't exist
    struct poll_data clients_data[fragment_count];

    // Store the number of currently-connected clients
    // Eventually, this will reach fragment count (but not exceed)
    int client_count = 0;
    int done_client_count = 0;

    // Open the BST
    void *root = NULL;
    ssort_open(&root);

    struct ssort_data *sorted_lines_data = malloc(sizeof(struct ssort_data) * INIT_SORTED_SIZE);
    char **sorted_lines = malloc(sizeof(char*) * INIT_SORTED_SIZE);
    unsigned int current_sorted_size = INIT_SORTED_SIZE;
    unsigned int sorted_line_count = 0;

    // Quit flag for when to stop looping
    int quit_flag = 0;

    while (!quit_flag) {
        // If there is at least one client connected,
        // Poll indefinitely until at least one event exists
        if ((err = poll_wait(epfd, &event_count, revents)) != SUCCESS) {
            ret(err, "Unable to poll for event");
        }

        // Loop through the events
        for (unsigned int i = 0; i < event_count; ++i) {
            struct poll_event revent = revents[i];
            struct poll_data *revent_data = revent.data;

            int revent_fd = revent_data->fd;
            enum poll_type revent_type = revent_data->type;
            uint32_t revent_flag = revent.event_flag;

            // Check if event is on the socket and it's a read event 
            if (revent_type == SOCKET) {
                if (revent_flag & POLL_DATA) {
                    int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientaddr_len);
                    int client_idx = client_count;
                    
                    // Reject clients if we've received the maximum amount
                    if (client_count == fragment_count) {
                        printf("Already have maximum number of clients\n");
                        printf("Ignoring new connection and continuing...\n");

                        close(clientfd);

                        continue;
                    }

                    if (clientfd == SOCK_ERR) {
                        ret(ERR_GENERIC, "The connection could not be accepted");
                    }

                    printf("Client successfully connected!\n");

                    // Store client data
                    clients_data[client_idx].fd = clientfd;
                    clients_data[client_idx].type = CLIENT;

                    // Add client to polling instance
                    if ((err = poll_add(epfd, clientfd, &clients_data[client_idx], POLL_DATA)) != SUCCESS) {
                        ret(err, "Unable to add client to polling instance");
                    }

                    // Write fragment data to client line-by-line
                    int fragmentfd = fragments[client_idx];

                    while (1) {
                        char *line = NULL;
                        unsigned int line_len = 0;

                        rw_readline(fragmentfd, &line, &line_len);
                        dbg_printf("line len: %u\n", line_len);

                        if (line_len == 0) {
                            break;
                        }

                        rw_write(clientfd, line, line_len);

                        line[line_len - 1] = '\0';
                        dbg_printf("read (%d): %s\n", line_len, line);

                        // Done sending data to client, so free the memory 
                        // used to read in from the fragment file.
                        free(line);
                    }

                    // Indicate the end of the message to the client
                    // (as in, all fragment lines have been written)
                    rw_writeend(clientfd);

                    dbg_printf("Wrote EOF!\n");
                    
                    // Increment client count
                    ++client_count;
                }
            // Otherwise, check if there is an event on a client
            } else if (revent_type == CLIENT) {
                // Check if the client has sent data
                if (revent_flag & POLL_DATA) {
                    // Parse a line from the client
                    char *line = NULL;
                    unsigned int line_number = 0;
                    unsigned int line_len = 0;

                    if ((err = fspec_parseline(revent_fd, &line, &line_number, &line_len)) != SUCCESS) {
                        ret(err,"Unable to parse line from client");
                    }

                    // If there is no more data to read, remove the client
                    if (line_len == 0) {
                        dbg_printf("Finished reading data from client\n");

                        // Close the client connection and remove it from the polling events
                        close(revent_fd);
                        poll_delete(epfd, revent_fd);

                        ++done_client_count;

                        // If all clients have been read, we're done!
                        if (done_client_count == fragment_count) {
                            quit_flag = 1;
                        }

                        // Otherwise, keep waiting for new data reads
                        continue;
                    }
                        
                    // Create a ssort_data struct for the read-in line
                    unsigned int line_idx = sorted_line_count;
                    sorted_lines_data[line_idx].key = line_number;
                    sorted_lines_data[line_idx].str = line;

                    // // Insert the struct into the tree for sorting
                    // ssort_insert(&root, &sorted_lines_data[line_idx]);

                    // Increment the number of sorted lines
                    ++sorted_line_count;

                    // Handling reallocating the current array of elements.
                    if(sorted_line_count == current_sorted_size){
                        dbg_printf("Resizing sorted_lines, sorted_lines_data\n");

                        current_sorted_size = current_sorted_size * RESIZE_FACTOR;
                        
                        sorted_lines_data = realloc(sorted_lines_data, sizeof(struct ssort_data) * current_sorted_size);

                        sorted_lines = realloc(sorted_lines, sizeof(char*) * current_sorted_size);
                    }
                }
            }
        }
    }

    // Insert all elements into the BST.
    for(unsigned int i = 0; i < sorted_line_count; i++){
        ssort_insert(&root, &sorted_lines_data[i]);
    }

    // Sort the entire tree.
    ssort_sort(&root, sorted_lines_data, sorted_lines, sorted_line_count);

    dbg_printf("Printing the entire poem...\n\n\n");
    
    for (unsigned int i = 0; i < sorted_line_count; ++i) {
        char *line = sorted_lines[i];
        char *line_text = line; // A ptr to find the first part of the line text.

        // Replace the newline with the null-terminating character.
        unsigned int len = 0;
        while (line[len] != LINE_TERMINATOR) {
            ++len;
        }
        line[len] = '\0';

        dbg_printf("%s\n", sorted_lines[i]);

        // See readme.txt, Attributions [1].
        // First, dereference line_text. If it is the null-termiantor, it 
        // evaluates to 0 (false) and the LOOP ends. We wish to remove all
        // numbers related to the line number but preserve spaces, tabs, 
        // indents, etc., so continue incrementing until it is not an 
        // ASCII number.
        while(*line_text && (*line_text >= '0' && *line_text <= '9')){
            line_text++;
        }
        // Increment once more because there is a space between line numbers and lines.
        // This is safe to do even if the line is empty, in which case it is the null-
        // terminator, and string-i/o will safely halt there.
        line_text++;
        
        fprintf(output, "%s\n", line_text);
    }

    ssort_close(&root); // Clean up anything the BST management lib created.
    
    // Free the pointers used to hold line data.
    for(unsigned int i = 0; i < sorted_line_count; i++){
        free(sorted_lines_data[i].str);
    }
    free(sorted_lines_data);
    free(sorted_lines);

    // Close file descriptors.
    for(int i = 0; i < fragment_count; i++){
        close(clients_data[i].fd);
        close(fragments[i]);
    }
    // Array of file descriptors was malloc'd in filespec.c. Done with them
    // here, so call free().
    free(fragments);

    close(sockfd);
    close(epfd);

    fclose(output);
    fclose(filespec); 

    return SUCCESS;
}
