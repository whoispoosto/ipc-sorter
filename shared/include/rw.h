#pragma once

// A character implementation is used to keep the code much simpler
// A string implementation (size > 1) could be added
// using memcmp, memcpy, etc.
#define NULL_TERMINATOR '\0'
#define LINE_TERMINATOR '\n'
#define NULL_TERMINATOR_SIZE 1
#define LINE_TERMINATOR_SIZE 1

enum ret_type rw_readline(int fd, char **buf, unsigned int *len);
enum ret_type rw_write(int fd, char *buf, unsigned int len);
enum ret_type rw_writeend(int fd);
