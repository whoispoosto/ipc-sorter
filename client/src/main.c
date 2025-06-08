#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rw.h"
#include "filespec.h"
#include "ssort.h"

#define ARG_PROGRAM_NAME 0
#define ARG_IP_ADDR 1
#define ARG_PORT_NUM 2
#define LISTEN_BACKLOG 50
#define WRITE_ONLY "w"
#define INIT_SORTED_SIZE 32
#define RESIZE_FACTOR 2
#define NUM_ARGS 3

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

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
    char* IP_ADDR;
    int PORT_NUM;

    if (argc < NUM_ARGS) {
        ret(ERR_NOT_ENOUGH_ARGS, "Please run: %s [IP address] [port]", argv[ARG_PROGRAM_NAME]);
    }

    IP_ADDR = argv[ARG_IP_ADDR];
    PORT_NUM = atoi(argv[ARG_PORT_NUM]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        handle_error("Failed to establish socket connection\n");
    }

    // Internet socket
    struct sockaddr_in addr;

    // reset struct to be all 0 and set the family to AF_UNIX
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_NUM);
    inet_aton(IP_ADDR, &addr.sin_addr);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        handle_error("Failed to connect to server");
    }

    printf("Client successfully connected!\n");

    void *root = NULL;
    ssort_open(&root);

    struct ssort_data *line_data = malloc(sizeof(struct ssort_data) * INIT_SORTED_SIZE);
    char **line_strs = malloc(sizeof(char*) * INIT_SORTED_SIZE);
    unsigned int current_line_count = INIT_SORTED_SIZE;
    unsigned int line_count = 0;

    while (1) {
        unsigned int line_idx = line_count;

        char *line = NULL;
        unsigned int line_number = 0;
        unsigned int line_len = 0;
        
        fspec_parseline(sockfd, &line, &line_number, &line_len);

        if (line_len == 0) {
            break;
        }

        // Fill in ssort_data struct
        line_data[line_idx].key = line_number;
        line_data[line_idx].str = line;

        // Store line
        line_strs[line_idx] = line;

        // Increment line count
        ++line_count;

        if(line_count == current_line_count){
            dbg_printf("Resizing line_data, line_strs!\n");

            current_line_count = current_line_count * RESIZE_FACTOR;
            
            line_data = realloc(line_data, sizeof(struct ssort_data) * current_line_count);

            line_strs = realloc(line_strs, sizeof(char*) * current_line_count);
        }
    }

    dbg_printf("Broke out of loop! Line count: %u\n", line_count);

    // Insert into tree
    for(unsigned int i = 0; i < line_count; i++){
        ssort_insert(&root, &line_data[i]);
    }
    // Sort the lines
    ssort_sort(&root, line_data, line_strs, line_count);

    // Write the sorted lines back to the server
    for (unsigned int i = 0; i < line_count; ++i) {
        // Get the line length
        // This needs to be re-computed since the line_strs array
        // is in a new order, so storing the line lengths wouldn't work.
        unsigned int line_len = 0;
        while (line_strs[i][line_len] != LINE_TERMINATOR) {
            ++line_len;
        }

        rw_write(sockfd, line_strs[i], line_len + LINE_TERMINATOR_SIZE);

        line_strs[i][line_len] = '\0';
        dbg_printf("Wrote: %s\n", line_strs[i]);
    }

    ssort_close(&root); // Clean up anything the BST management lib created.

    // Free the pointers used to hold line data.
    for(unsigned int i = 0; i < line_count; i++){
        free(line_data[i].str);
    }
    free(line_data);
    free(line_strs);

    close(sockfd);

    return 0;
}
