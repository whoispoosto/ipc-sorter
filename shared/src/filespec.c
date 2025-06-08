#include "filespec.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "rw.h"

#define F_READ "r"
#define F_WRITE "w"
#define F_START 0
#define F_ERR -1
#define BUF_SIZE 512
#define INITIAL_FRAGMENT_COUNT 4
#define FRAGMENT_FACTOR 2
#define INITIAL_LINE_COUNT 1
#define LINE_FACTOR 2

enum ret_type fspec_open(FILE **filespec, char *pathname) {
    *filespec = fopen(pathname, F_READ);

    if (*filespec == NULL) {
        return ERR_UNABLE_TO_OPEN_FILE;
    }

    return SUCCESS;
}

enum ret_type fspec_openoutput(FILE *filespec, FILE **output) {
    // Start at top of file
    if (fseek(filespec, F_START, SEEK_SET) == F_ERR) {
        return ERR_FILE_OP;
    }

    // Read first line
    char pathname[BUF_SIZE];
    memset(pathname, 0, BUF_SIZE);
    fgets(pathname, BUF_SIZE, filespec);

    // Replace newline with null terminating character
    pathname[strcspn(pathname, "\n")] = '\0';

    // Open output file for writing
    *output = fopen(pathname, F_WRITE);

    if (*output == NULL) {
        return ERR_UNABLE_TO_OPEN_FILE;
    }

    return SUCCESS;
}

// in main: FILE *fragments[];
// in this func: *fragments = [some array of file pointers];
enum ret_type fspec_openfragments(FILE *filespec, int *fragments[], int *fragment_count) {
    // Start at top of file
    if (fseek(filespec, F_START, SEEK_SET) == F_ERR) {
        return ERR_FILE_OP;
    }

    enum ret_type RETURN_VALUE = SUCCESS;

    // Read first line
    // We don't care about the output,
    // we just need to skip the first line.
    // Guaranteed that this first line is the output file name per the 
    // assignment instructions.
    char temp[BUF_SIZE];
    fgets(temp, BUF_SIZE, filespec);

    // Read line fragments
    // We start with an initial array
    // Assumption: no line is longer than BUF_SIZE
    int fcount = INITIAL_FRAGMENT_COUNT;
    char **pathnames = malloc(fcount * sizeof(char *));
    int curr_fragment = 0;

    // Allocate an initial buffer
    // We will allocate the rest in the loop itself
    pathnames[curr_fragment] = malloc(BUF_SIZE);

    // Go through every remaining line in the filespec
    while (fgets(pathnames[curr_fragment], BUF_SIZE, filespec) != NULL) {
        // Remove the newline
        char *pathname = pathnames[curr_fragment];
        pathname[strcspn(pathname, "\n")] = '\0';

        // Increment curr fragment
        ++curr_fragment;

        // If we need to allocate more memory
        if (curr_fragment >= fcount) {
            fcount *= FRAGMENT_FACTOR;
            pathnames = realloc(pathnames, fcount * sizeof(char *));
        }

        // Malloc next buffer so we are one ahead
        pathnames[curr_fragment] = malloc(BUF_SIZE);
    }

    // At the end of the loop, get rid of redundant memory
    // First, free the unused buffer
    free(pathnames[curr_fragment]);

    // Then, truncate the array
    fcount = curr_fragment;
    pathnames = realloc(pathnames, fcount * sizeof(char *));

    // Allocate array
    *fragments = malloc(fcount * sizeof(int));

    // Convert pathnames to file descriptors
    for (int i = 0; i < fcount; ++i) {
        char *pathname = pathnames[i];
        int fd = open(pathname, O_RDONLY);

        if (fd == F_ERR) {
            RETURN_VALUE = ERR_UNABLE_TO_OPEN_FILE;
        }
        
        // No longer need the pathname, so may be freed here.
        free(pathname);
        // Store file descriptor
        (*fragments)[i] = fd;
    }

    // Store fragment_count in pointer
    *fragment_count = fcount;

    // Free the array of pathnames.
    free(pathnames);

    return RETURN_VALUE;
}

enum ret_type fspec_parseline(int fragment, char **line, unsigned int *line_number, unsigned int *line_length) {
    char *temp_line = NULL;
    unsigned int temp_length = 0;

    // Read in an entire line
    rw_readline(fragment, &temp_line, &temp_length);

    // Don't try to parse empty lines
    // This isn't considered an error--the line is simply empty
    // (perhaps EOF has been reached)
    if (temp_length == 0) {
        return SUCCESS;
    }

    // Parse line
    sscanf(temp_line, "%u", line_number);

    // Store line and line length
    *line = temp_line;
    *line_length = temp_length;

    return SUCCESS;
}
