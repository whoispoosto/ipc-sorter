#define _GNU_SOURCE

#include <search.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ssort.h"

#define LESS_THAN -1
#define GREATER_THAN 1
#define EQUAL_TO 0

static unsigned int idx = 0;

static int compare(const void *pa, const void *pb) {
    unsigned int ka = ((struct ssort_data *)pa)->key;
    unsigned int kb = ((struct ssort_data *)pb)->key;

    if (ka < kb) {
        return LESS_THAN;
    } else if (ka > kb) {
        return GREATER_THAN;
    } else {
        return EQUAL_TO;
    }

    return SUCCESS;
}

/* 
"For the data in each tree node the function free_node is 
called. The pointer to the data is passed as the argument 
to the function. If no such work is necessary, 
free_node must point to a function doing nothing." 
    - tdestroy() man page
*/
static void free_node(void *nodep) {}

static void action(const void *nodep, VISIT which, int depth) {
    struct ssort_data *data; 

    switch (which) {
        // We only care about leaf and postorder
        // since we want to get the BST in sorted order.
        case leaf:
        case postorder:
            data = *(struct ssort_data **)nodep;

            data->sorted_idx = idx;
            ++idx;

            break;
        case preorder:
            break;
        case endorder:
            break;
    }
}

enum ret_type ssort_open(void **root) {
    *root = NULL;

    return SUCCESS;
}

enum ret_type ssort_close(void **root) {
    tdestroy(*root, free_node);

    return SUCCESS;
}

enum ret_type ssort_insert(void **root, struct ssort_data *data) {
    tsearch(data, root, compare);

    return SUCCESS;
}

enum ret_type ssort_sort(void **root, struct ssort_data data[], char *sorted_strings[], unsigned int count) {
    // Reset global index 
    // This is needed since the action callback to twalk can't take in any context.
    // So, we use this global variable to keep track of the current node
    // to get them in order.
    idx = 0;
    twalk(*root, action);

    // Iterate over all inserted elements
    for (unsigned int i = 0; i < count; ++i) {
        // Get an element from the tree
        struct ssort_data **element_ptr = tsearch(&data[i], root, compare);
        struct ssort_data *element = *element_ptr;

        // Insert the element into the sorted strings array based on its sorted index
        // This is analogous to bucket sort
        unsigned int bucket = element->sorted_idx;
        char *str = element->str;
        
        sorted_strings[bucket] = str;
    }

    return SUCCESS;
}


