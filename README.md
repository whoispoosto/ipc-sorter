## IPC Sorter: Multiplexed Inter-Process Communication
*Note: this project was copied from its original private repository, hence the lack of commit history.

**Students:**  
Preston Meek, meek@wustl.edu  
Chiagozie Okoye, c.i.okoye@wustl.edu

---

## Server Design

Beginning at Step 3, the server performs basic checks of command line arguments passed to it on invocation. In building this approach, we defined an enum `ret_type` with various return values, this defined in `/server/include/util.h`.

For the functionality of the server's setup with the fragment files, `/server/src/filespec.c` was made to write helper functions that declutter `main.c`.

Dynamic memory was used to allow for an arbitrary count of fragment files. We hold a dynamically allocated array of pointers to chars with space for 4 pointers to fragment file path names. In a loop, the program continually reads available lines from the filespec, keeping track of how many files it has read and how much space is in the array of pointers. If there is a new line and no space in the array, the array's size is doubled; otherwise, the next pointer in the array is updated to be the pointer returned by `malloc()` for the subsequent string. At the end of this loop, any additional, unused memory is tripped with `realloc()`. The final steps of this helper function open the files associated with the file names and stores them into an array of file descriptors defined in `main()`.

Step 4's implementation of the socket followed as in studios, using a helper function in `/server/src/socket.c` to handle setting up the socket, returning the fd to `main()`.

For I/O multiplexing, Linux's epoll interface was chosen. Originally, poll was our choice, poll chosen over `select()` because poll neither required `select()`'s `fd_set` to be reinitialized after each call nor the work of indirectly checking values between 0 and nfds. In the end, though, `epoll()` was chosen over `poll()` because `epoll()` allows data to be packaged into the elements it will keep track of; in our case, a file descriptor. Storing the fds into the epoll structs was simpler because `poll()` would require us to manage separate array(s).

Back in `main()`, the server waits for connections, alerted of such by `epoll()`. When a connection is made, the server accepts it, but before doing anything else, it checks that it has not already accepted the maximum needed connections (the total number of fragment files). If it has already accepted enough clients, the server closes the connection, causing the client to also quit (more on this later). In the normal case of the server accepting a valid client, the serve writes work to the client, the work being the contents of a file fragment.

During the acquisition of the connection, the server also adds the clients to the epoll instance to be tracked for I/O. When the client sends data, `epoll()` returns and the server handles reading in data. Reads and writes are handled by a helper function defined in `/shared/src/rw.c`. For each number—line pair, the server adds it to a dynamically allocated array that will be handled by a BST management library provided by `<search.h>`. When all data has been read in from a client, the server removes the client from the list of items to be checked by `epoll()`.

When all clients have returned with data, the `while()` loop is broken. The server now performs the task of merging the sorted data, but this was handled by the BST tree via insertions from the dynamically allocated array into teh BST. After insertion, server can perform an in-order traversal, `ssort_sort()`, which relies on `twalk()` from `<search.h>`. With all the data sorted, it is only a matter of writing the data line-by-line to the specified output file.

## Client Design

Much like the server, the client begins with parsing command line arguments. The arguments are used to create a socket that connects to the server's socket. Without need for I/O multiplexing, the client can sit and wait for data from its connection. The client reads in data, ensures it has a line number and line, and, much like the server, stores the number-line pair in a dynamically allocated array. This array is passed to the `ssort_sort()` to sort the data using a BST data structure. Once this work has been completed, the client writes the ordered data back to the server over the socket.

## Build Instructions

Provided with this repository is a Makefile located in the main working directory. When the command `make` is issued, all .c files in respective `/src` directories are compiled into executables stored in `/build` folders. The executable `bin_server` runs the server and `bin_client` runs the client.

## Testing & Evaluation

A shell script was written to aid in testing usage. It is located in the main directory, `test_lab_usage.sh`. Running the script runs several test cases all related to the proper usage of the `bin_server` and `bin_client` executables. Running them on the shell generates no errors, as expected.
