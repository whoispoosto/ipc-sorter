#pragma once

#include <stdint.h>
#include "util.h"

#define FRAGMENT_SIZE 512
#define MAX_EVENT_COUNT 25

#define POLL_DATA EPOLLIN

enum poll_type {
    CLIENT,
    SOCKET
};

struct poll_data {
    int fd;
    enum poll_type type;
};

struct poll_event {
    uint32_t event_flag;
    struct poll_data *data;
};

enum ret_type poll_open(int *epfd);
enum ret_type poll_add(int epfd, int fd, struct poll_data *data, uint32_t event_flag);
enum ret_type poll_delete(int epfd, int fd);
enum ret_type poll_wait(int epfd, unsigned int *event_count, struct poll_event events[MAX_EVENT_COUNT]);
