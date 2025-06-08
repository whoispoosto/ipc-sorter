#pragma once

#include "util.h"

struct ssort_data {
    // Input
    unsigned int key;
    char *str;

    // Output
    unsigned int sorted_idx;
};

enum ret_type ssort_open(void **root);
enum ret_type ssort_close(void **root);
enum ret_type ssort_insert(void **root, struct ssort_data *data);
enum ret_type ssort_sort(void **root, struct ssort_data data[], char *sorted_strings[], unsigned int count);
