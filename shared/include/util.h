#pragma once

enum ret_type {
    SUCCESS,
    ERR_GENERIC,
    ERR_NOT_ENOUGH_ARGS,
    ERR_UNABLE_TO_OPEN_FILE,
    ERR_FILE_OP,
    ERR_UNABLE_OPEN_SOCKET,
    ERR_UNABLE_BIND_SOCKET,
    ERR_UNABLE_LISTEN_SOCKET,
    ERR_NULL_PTR,
    ERR_EPOLL_INVALID,
    ERR_EPOLL_CREATE_FAIL,
    ERR_EPOLL_MOD_FAIL,
    ERR_EPOLL_WAIT_FAIL
};

// Attribution [4]
// Macro to return based on a provided error code.
// Uses variable args for printf formatting.

// Note: string literals in C that are next to each other get concatenated.
// Example: "hello" " world" --> "hello world"
// In this case, str "\n" --> "[expanded str]\n"
#define ret(err, str, ...) do { \
    printf("ERROR: " str "\n", ##__VA_ARGS__); \
    printf("Exiting with error code %d\n", err); \
    exit(err); \
} while (0);
