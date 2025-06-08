# File: test_lab.sh
# Students: Preston Meek, Chiagozie Okoye

# A function to check and terminate bin_server if it's running.
function kill_server {
    if pgrep bin_server > /dev/null; then
        pkill bin_server > /dev/null
    fi
}


# SET UP
SUCCESS=0
ERR_GENERIC=1
ERR_NOT_ENOUGH_ARGS=2
ERR_UNABLE_TO_OPEN_FILE=3

IP_ADDR="0.0.0.0"
PORTNUM=31133
echo "Running tests on CSE422S Lab 3. Apart from headers, this script only prints on errors."

# Tests 1-4: Proper Usage

echo "Tests 1-4: Usage"

# Redirect output to /dev/null, a file that cannot be written to, so that
# terminal output remains clean.
./bin_server > /dev/null 
result=$?

if [ $result -ne $ERR_NOT_ENOUGH_ARGS ]; then
    echo "ERROR TEST 1: server ran w/o proper usagae!"
fi

# Specify a non-existent file.
./bin_server test0/not_real $PORTNUM > /dev/null
result=$?

if [ $result -ne $ERR_UNABLE_TO_OPEN_FILE ]; then
    echo "ERROR TEST 2: server ran w/o real file, Status: $result!"
fi

./bin_client > /dev/null
result=$?

if [ $result -ne $ERR_NOT_ENOUGH_ARGS ]; then
    echo "ERROR TEST 3: client ran w/o proper usage!"
fi

./bin_client $IP_ADDR > /dev/null
result=$?

if [ $result -ne $ERR_NOT_ENOUGH_ARGS ]; then
    echo "ERROR TEST 4: client ran w/o proper usage!"
fi


# Test 5: A working server and one client.
# echo "Test 5: One Client"

# # Run the server with proper args in the background.
# ./bin_server test0/jabberwocky_spec $PORTNUM > /dev/null &

# ./bin_client $IP_ADDR $PORTNUM

# # The server should still be running after receiving one client
# if ! pgrep bin_server > /dev/null; then
#     echo "ERROR TEST 5: server not running when work still available."
# fi
# kill_server # Terminate the server before moving on.


# # Test 6: A working server and enough clients to do all work.
# echo "Test 6: Normal Case"

# ./bin_server test0/jabberwocky_spec $PORTNUM > /dev/null &

# num_lines=$(wc -l < "test0/jabberwocky_spec") # Redirect to get just count.
# num_fragments=$(( $num_lines - 1 ))

# for ((i = 0 ; i < num_fragments ; i++)); do
#   ./bin_client $IP_ADDR $PORTNUM &
# done

# result=$?

# if [ $result -ne $SUCCESS ]; then
#     echo "ERROR TEST 6: server did not complete regular execution! 
#     Exited with $result"
# fi

# CLEAN UP
kill_server

echo "All tests completed!"
