#include "poll.h"

#include <string.h>
#include <sys/epoll.h>

#define EPOLL_ERR -1
#define POLL_INDEFINITELY -1

enum ret_type poll_open(int *epfd) {
    // Create an epoll instance
    int temp_epfd = epoll_create1(0);

    // Handle errors
    if (temp_epfd == EPOLL_ERR) {
        return ERR_EPOLL_CREATE_FAIL;
    }

    // Store the polling fd
    *epfd = temp_epfd;
    return SUCCESS;
}

enum ret_type poll_add(int epfd, int fd, struct poll_data *data, uint32_t event_flag)  {
    if (epfd == EPOLL_ERR) {
        return ERR_EPOLL_INVALID;
    } 

    // It's perfectly safe to create this within the stack frame
    // since epoll_ctl copies it.
    // The poll_data is NOT copied, though, which is why
    // we use a pointer for it.
    struct epoll_event event;

    event.events = event_flag;
    event.data.ptr = data;

    if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == EPOLL_ERR) {
        return ERR_EPOLL_MOD_FAIL;
    }

    return SUCCESS;
}

enum ret_type poll_delete(int epfd, int fd) {
    if (epfd == EPOLL_ERR) {
        return ERR_EPOLL_INVALID;
    }

    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == EPOLL_ERR) {
        return ERR_EPOLL_MOD_FAIL;
    }

    return SUCCESS;
}

enum ret_type poll_wait(int epfd, unsigned int *event_count, struct poll_event events[MAX_EVENT_COUNT]) {
    int ecount;
    struct epoll_event revents[MAX_EVENT_COUNT];

    if ((ecount = epoll_wait(epfd, revents, MAX_EVENT_COUNT, POLL_INDEFINITELY)) == EPOLL_ERR) {
        return ERR_EPOLL_WAIT_FAIL;
    }

    // Store event count
    *event_count = ecount;

    // Reset events output
    memset(events, 0, sizeof(struct poll_event) * MAX_EVENT_COUNT);

    // Store all events
    for (int i = 0; i < ecount; ++i) {
        struct epoll_event revent = revents[i];

        events[i].event_flag = revent.events;
        events[i].data = revent.data.ptr;
    }

    return SUCCESS;
}
