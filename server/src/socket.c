#include "socket.h"

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 50
#define MEM_RESET 0
#define PROTOCOL_DEFAULT 0

enum ret_type socket_open(int *fd, int port) {
    // Create and open a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, PROTOCOL_DEFAULT);

    if (sockfd == SOCK_ERR) {
        return ERR_UNABLE_OPEN_SOCKET;
    }

    // Internet socket
    struct sockaddr_in addr;

    // reset struct to be all 0
    memset(&addr, MEM_RESET, sizeof(addr));

    // set the family to AF_INET
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // Cast to generic socket and bind
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == SOCK_ERR) {
        return ERR_UNABLE_BIND_SOCKET;
    }

    // Start listening on server
    if (listen(sockfd, LISTEN_BACKLOG) == SOCK_ERR) {
        return ERR_UNABLE_LISTEN_SOCKET;
    }

    printf("IP Address: %s, Listening on Port: %d\n", inet_ntoa(*(struct in_addr *)&addr.sin_addr.s_addr), port);

    // Store file descriptor
    *fd = sockfd;

    return SUCCESS;
}
