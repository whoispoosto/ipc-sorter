#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rw.h"
#include "util.h"

#define INITIAL_SIZE 1
#define RESIZE_FACTOR 2

// Attribution [3]
//#define DEBUG // uncomment this line to enable debugging

#ifdef DEBUG
/* When debugging is enabled, these form aliases to useful functions */
#define dbg_printf(...) printf(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated for these */
#define dbg_printf(...)
#endif


static enum ret_type rw_readuntil(int fd, char **buf, unsigned int *len, char delim) {
    // Setup initial size and index
    unsigned int curr_size = INITIAL_SIZE;
    unsigned int curr_idx = 0;

    // Store number of bytes read per iteration
    ssize_t bytes_read;

    // Setup line-parsing flags
    int delim_found = 0;
    int null_terminator_found = 0;

    // Malloc initial buffer
    char *temp_buf = malloc(curr_size);
    dbg_printf("Initial malloc of size %u\n", curr_size);

    // Read data byte-by-byte by passing specifying the count argument to be 1.
    while ((bytes_read = read(fd, temp_buf + curr_idx, 1)) != 0) {
        // Check if the null terminator has been reached
        unsigned int prev_idx = curr_idx;

        // Loop over all new characters
        while (curr_idx < prev_idx + bytes_read)  {
            // Check if the delimiter is found
            if (temp_buf[curr_idx] == delim) {
                delim_found = 1;
                break; 
            }

            // Regardless of what the delimiter is,
            // always check if the null terminator has been found
            if (temp_buf[curr_idx] == NULL_TERMINATOR) {
                null_terminator_found = 1;
                break;
            }

            ++curr_idx;
        }

        // If one of the special characters are found, return early
        // Otherwise, continue in the loop
        if (delim_found || null_terminator_found) {
            break;
        }

        // Check if we have reached the end of the buffer
        if (curr_idx == curr_size) {
            // Increase buffer size and realloc buffer
            curr_size *= RESIZE_FACTOR;
            temp_buf = realloc(temp_buf, curr_size);
            dbg_printf("Realloc to size %u\n", curr_size);
        }
    }

    // Return early on empty reads or when the end of a packet is reached
    if (curr_idx == 0 || null_terminator_found) {
        
        // At least one byte is allocated for reads, so free it.
        free(temp_buf);
        
        *buf = NULL;
        *len = 0;        
        return SUCCESS;
    }

    // Final reallocation to either:
    // 1) have 1 extra byte for null terminating character, or
    // 2) remove extraneous bytes from buffer
    //
    // We MUST INCLUDE THE NEWLINE IN THE FINAL MESSAGE
    // This is how the client knows the packet is done.
    // Think magic number from 132, but at the end
    // instead of at the front.
    //
    // An alternative would be to have a more formalized packet structure
    // with a starting magic number, string length, etc.
    // but this is the simplest approach.
    curr_size = curr_idx + LINE_TERMINATOR_SIZE;
    temp_buf = realloc(temp_buf, curr_size);

    // Replace whatever delimiter we found with the line terminator
    temp_buf[curr_idx] = LINE_TERMINATOR;

    // We must also null-terminate the string so that stdlib functions
    // know when to stop reading.
    int total_size = curr_size + NULL_TERMINATOR_SIZE;
    temp_buf = realloc(temp_buf, total_size);

    // Add the null-terminator at the end of the string.
    temp_buf[curr_size] = NULL_TERMINATOR;

    dbg_printf("Final realloc to size %u\n", curr_size);
    dbg_printf("Final buffer: %s\n", temp_buf);

    // Store buffer and length
    *buf = temp_buf;
    *len = curr_size;

    return SUCCESS;
}

enum ret_type rw_readline(int fd, char **buf, unsigned int *len) {
    return rw_readuntil(fd, buf, len, LINE_TERMINATOR);
}

enum ret_type rw_write(int fd, char *buf, unsigned int len) {
    // Stole number of bytes written per iteration
    ssize_t bytes_written = 0;

    while (bytes_written < len) {
        bytes_written += write(fd, buf + bytes_written, len - bytes_written);
    }

    return SUCCESS;
}

enum ret_type rw_writeend(int fd) {
    // Since the current implementation simply uses a character for the null terminator,
    // we need to convert it into a character array (aka a string/buffer)
    char buf[NULL_TERMINATOR_SIZE];

    // Since it's a single character,
    // all we have to do is assign index 0 of the buffer
    buf[0] = NULL_TERMINATOR;

    write(fd, buf, NULL_TERMINATOR_SIZE);

    return SUCCESS;
}
