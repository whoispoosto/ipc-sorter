CSE 422S - Operating Systems Organization
Lab 3: Multiplexed Inter-Process Communication

Students:
Preston Meek, meek@wustl.edu
Chiagozie Okoye, c.i.okoye@wustl.edu

*** Server Design ***
    Beginning at Step 3, the server performs basic checks of command line
arguments passed to it on invocation. In building this approach, we defined an enum ret_type
with various return values, this defined in /server/include/util.h.
    For the functionality of the server's setup with the fragment files,
/server/src/filespec.c was made to write helper functions that declutter main.c.
    Dynamic memory was used to allow for an arbitrary count of fragment files.
We hold a dynamically allocated array of pointers to chars with space for
4 pointers to fragment file path names. In a loop, the program continually reads
available lines from the filespec, keeping track of how many files it has read and
how much space is in the array of pointers. If there is a new line and no space
in the array, the array's size is doubled; otherwise, the next pointer in the
array is updated to be the pointer returned by malloc() for the subsequent string.
At the end of this loop, any additional, unused memory is tripped with realloc().
The final steps of this helper function open the files associated with the file
names and stores them into an array of file descriptors defined in main().
    Step 4's implementation of the socket followed as in studios, using a helper
function in /server/src/socket.c to handle setting up the socket, returning the fd
to main().
    For I/O multiplexing, Linux's epoll interface was chosen. Originally, poll was
our choice, poll chosen over select() because poll neither required select()'s
fd_set to be reinitialized after each call nor the work of indirectly checking values
between 0 and nfds. In the end, though, epoll() was chosen over poll() because
epoll() allows data to be packaged into the elements it will keep track of; in
our case, a file descriptor. Storing the fds into the epoll structs was simpler
because poll() would require us to manage separate array(s).
    Back in main(), the server waits for connections, alerted of such by epoll().
When a connection is made, the server accepts it, but before doing anything else,
it checks that it has not already accepted the maximum needed connections (the total
number of fragment files). If it has already accepted enough clients, the server
closes the connection, causing the client to also quit (more on this later). In
the normal case of the server accepting a valid client, the serve writes work
to the client, the work being the contents of a file fragment.
    During the acquisition of the connection, the server also adds the clients to
the epoll instance to be tracked for I/O. When the client sends data, epoll()
returns and the server handles reading in data. Reads and writes are handled
by a helper function defined in /shared/src/rw.c. For each number—line pair, the
server adds it to a dynamically allocated array that will be handled by a BST
management library provided by <search.h>. When all data has been read in from
a client, the server removes the client from the list of items to be checked by
epoll().
    When all clients have returned with data, the while() loop is broken. The
server now performs the task of merging the sorted data, but this was handled by
the BST tree via insertions from the dynamically allocated array into teh BST.
After insertion, server can perform an in-order traversal, ssort_sort(), which
relies on twalk() from <search.h>. With all the data sorted, it is only a
matter of writing the data line-by-line to the specified output file.


*** Client Design ***
    Much like the server, the client begins with parsing command line arguments.
The arguments are used to create a socket that connects to the server's socket.
Without need for I/O multiplexing, the client can sit and wait for data from its
connection. The client reads in data, ensures it has a line number and line,
and, much like the server, stores the number-line pair in a dynamically
allocated array. This array is passed to the ssort_sort() to sort the data
using a BST data structure. Once this work has been completed, the client
writes the ordered data back to the server over the socket.


*** Build Instructions ***
Provided with this repository is a Makefile located in the main working
directory. When the command 'make' is issued, all .c files in respective /src
directories are compiled into executables stored in /build folders. The
executable `bin_server` runs the server and `bin_client` runs the client.


*** Testing & Evaluation ***
A shell script was written to aid in testing usage. It is located in the main 
directory, test_lab_usage.sh. Running the script runs several test cases all
related to the proper usage of the bin_server and bin_client executables.
Running them on the shell generates no errors, as expected.
    Now, for more elaborate tests. There are four stages of testing, each with a
normal and unique test case. Each stage is contained in a directory, testX/,
where X is an integer. Each directory has a different number of fragment files
as well as different contents, generated by the provided file_shuffle_cut.cpp.
The directories also contain personal shell scripts that can be run from the
main repository directory such as ./testX/test_server.sh. For these tests, one
terminal was used to run the server script while another ran the client(s).
    Test 0 (in test0/) begins with a simple run to completion. Several clients are
spawned concurrently by using the `&` after the executable. This returns
successfully, outputting the file "jabberwocky_out."
    Additionally, for this directory, the unique test is to see what happens to
the server when more clients than are needed attempt to connect.
test0/test_too_many_clients.sh has several clients attempt to connect. The
server accepts the first 5, corresponding to the number of fragment files to
sort, but it rejects following ones, printing this fact out:
"""
$ ./test0/test_server.sh
    IP Address: 0.0.0.0, Listening on Port: 31133
    Client successfully connected!
    Client successfully connected!
    Client successfully connected!
    Client successfully connected!
    Client successfully connected!
    Already have maximum number of clients
    Ignoring new connection and continuing...
    Already have maximum number of clients
    Ignoring new connection and continuing...
    Already have maximum number of clients
    Ignoring new connection and continuing...
"""

    Test 1 (/test1) follows right after its predecessor in terms of approach. The
server and client cooperate to generate a valid, reconstructed file from the
cut up file_shuffle_cut.cpp that was broken into fragments by that very program.
The program runs to completion and exits with success.
    Alongside the normal expected concurrent execution is a sequential version
that does the same thing as the basic version, but does not utilize the `&`.
A shell call to `sleep` is also utilized to hammer this home. This test was to
check that the server, upon finishing all read-writes transactions with a
client, still remains open to connections if it has work to do. The server
should wait indefinitely for new connections until it has divvied out all work,
and the command calls to sleep simulate this waiting. This test runs to
completion and properly generates a reconstructed file_shuffle_cut.cpp_out.
    Test 2 uses a draft of this very readme as input to the provided helper C++
program. Further, the test features double the amount of fragment files than
Test 0 in an attempt to test the dynamic allocation of space for file
descriptors held by the server and client programs.
The normal run of Test 2 generates the correct expected output. For this test's
special run, there is a separate shell script that runs a poorly formed file
specification. The assignment writeup instructions specify a distinct form for
the filespec that implies file names will be consecutive and without line
breaks. Passing a malformed filespec with line breaks does not crash the
program, but the program does return (as intended) with error.
"""
    $ ./test2/test_server_bad_spec.sh
    ERROR: Unable to open fragment files
    Exiting with error code 3
"""

    The final stage of testing is Test 3 (/test3). This stage has 12 fragment files
that are the contents of the Makefile used to build this project. Executing the
normal phase with test_server.sh returns successfully. For this stage, there
are two special cases that were tested.
    The first special case builds very closely off the special test case from Stage
2, but instead of blank lines, the file spec contains nonexistent files. The
server handles this gracefully, returning with error.
"""
    $ ./test3/test_server_DNE_files.sh
    ERROR: Unable to open fragment files
    Exiting with error code 3
"""
    The second special case notes the different expected behavior for reading in
lines from the fragment files. Unlike the filespec, the fragment files are not
guaranteed to have consecutive lines. So, there may be empty lines without any
text at all that don't correspond to valid lines in the input source and
expected output. The server and client code should, however, be able to handle
blank lines. This is tested by manually adding excessive, random blank lines
throughout the 12 fragment files. The program still runs to completion, and
Makefile_reconstructed possesses the same format as its original source.

    In the finishing steps of this lab, there was only really one major bug that
was difficult to solve: dynamic memory and the BST. In the initial development,
all data regarding parsed lines was allocated statically to quickly develop a
working product. There was also a slightly different program flow: when the
server read in a sorted line from the client, it would immediately construct a
BST node and insert it into the tree. This worked seamlessly for the static
implementation where a large, pre-defined array was allocated globally, but
would spell trouble for the dynamic approach.
    Changing the instantiations of our arrays was not difficult with a call to
malloc. Additionally, resizing the malloc'd array is easy: just call realloc()
once the program has reached the end of the current array. Here, though, is
where our program went to die. The program would run without error up until the
point it had to print out the results of the reconstructed file: it would
receive a SIGSEGV signal and crash. Using gdb, it was discovered that,
somewhere, the BST did not possess certain lines (e.g., line 4 from the expected
output would be missing from the BST). This meant that when the BST performed
the sort and wrote the lines to their correct indices in the sorted array, an
index would be unwritten to and have the value NULL. A NULL dereference will
naturally lead to a segmentation fault, but what confounded group members was
why this was happening.
    gdb was used to print values and backtrace. Specifically, the dynamically
allocated array of the BST nodes was printed out. In doing so, it was noted
that some of the values in the malloc'd pointer were not being properly updated
by the BST as they should be. For example, many nodes had the same sorted_idx
member variable, which is not expected after a call to ssort_sort(). ssort_sort
() was supposed to write the correct sorted index to this variable. That is
when it was realized that the BST library was not updating the correct objects
in memory, and this was all because of the reallocation step. At the onset,
a size is dynamically allocated for the array of BST nodes. The BST library
accepts pointers to the elements of this array with ssort_insert(), and the
library only works on these pointers. These pointers, though, are not updated
when a reallocation happens. The memory allocator realloc(), in trying to find
a larger or smaller chunk of memory, might decide to pick a different memory
location. realloc() will copy any previous memory into this new location, which
is entirely fine. What is NOT fine is the set of pointers that exist in the BST.
These pointers are no longer valid after a realloc(); surprisingly, though, the
system did not throw a fault when the invalid pointers passed to the BST before
realloc() were used.
    Originally, it was unsure how to solve this issue. Group members were tempted
to abandon a dynamic approach. But, upon querying ChatGPT—who, unhelpfully,
provided useless solutions—members realized that the BST did not need to accept
pointers to nodes immediately when those nodes are constructed. They could,
instead, be inserted into the BST at the very end, after all calls to realloc().
There already was a dynamically allocated array of all the (unsorted) nodes, so
all was needed was to iterate over this array and call ssort_insert(). This
would ensure that all pointers to nodes would be valid to be used by the BST
library. This change in approach successfully resolved the fault.


*** Development Effort ***
Estimated Total Person Working Time: 30 hours.

*** Attributions ***
The following is a list of attributions to resources used in completing this lab.
This includes references to the textbook and responses from generative AI 
such as ChatGPT. For the generative AI case, the pasted text is shortened to 
the most relevant parts for readability (including emojis ChatGPT generated 
was viewed as unnecessary). When code generated by AI is used in the source 
files, it is commented with student understandings.

[1] Pointer-Walking for fprintf()
Prompt:
"""
    With the following, how can I change the sscanf() call to maintain 
    any spaces and tabs after the first space between the line number and the 
    string?
        sscanf(line, "%u %[^\n]", &line_number, line_text);
        fprintf(output, "%s\n", line_text);
    The value of line is something like the following:

    0 Hello, it is a
    1              wonderful day!
    2 What can we do
    3              on this beautiful day?
"""

ChatGPT:
"""
    unsigned int line_number;
    char line[256];
    char line_text[256];

    // Suppose you read a line into `line` here

    char *p = line;
    sscanf(p, "%u", &line_number);

    // Skip digits
    while (*p && (*p >= '0' && *p <= '9'))
        p++;

    // Now `p` is at the first space or tab after the number

    // DO NOT skip spaces or tabs — we want to keep them
    // Just copy everything from here
    strcpy(line_text, p);

    // Example: write it back
    fprintf(output, "%s\n", line_text);
"""


[2] Makefile
Prompt: 
"""
    can you make me a super basic makefile? source files in src folder, 
    header files in include folder. For c not C++
"""

ChatGPT:
"""
    # Compiler and flags
    CXX := g++
    CXXFLAGS := -Iinclude -Wall -Wextra -std=c++17

    # Source and object files
    SRC := $(wildcard src/*.cpp)
    OBJ := $(SRC:src/%.cpp=build/%.o)

    # Output binary
    TARGET := main

    # Default rule
    all: $(TARGET)

    # Link object files to create executable
    $(TARGET): $(OBJ)
        $(CXX) $(OBJ) -o $@

    # Compile .cpp to .o
    build/%.o: src/%.cpp | build
        $(CXX) $(CXXFLAGS) -c $< -o $@

    # Create build directory if it doesn't exist
    build:
        mkdir -p build

    # Clean up build artifacts
    clean:
        rm -rf build $(TARGET)

    .PHONY: all clean
"""


[3] dbg_printf()
This was borrowed from the source code provided in the fifth lab
assignment from WashU's CSE 361: Introduction to Systems Software course,
specifically the version from Spring 2024.

[4] Error Return Macro
Inspired by Kerrisk's The Linux Programming Interface.
