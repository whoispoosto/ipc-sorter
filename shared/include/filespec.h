#pragma once

#include <stdio.h>

#include "util.h"

// TODO: change all from FILE * to fd
// Using FILE * is fine for filespec, since it's a file.
// But since we are working with servers,
// we can't reliably receive an EOF since the connection can stay open.

enum ret_type fspec_open(FILE **filespec, char *pathname);
enum ret_type fspec_openoutput(FILE *filespec, FILE **output);
enum ret_type fspec_openfragments(FILE *filespec, int *fragments[], int *fragment_count);
enum ret_type fspec_parseline(int fragment, char **line, unsigned int *line_number, unsigned int *line_length);
