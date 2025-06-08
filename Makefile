# Attribution [2]

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -g -std=gnu17 -Ishared/include

# Output binary
SERVER_TARGET := bin_server
CLIENT_TARGET := bin_client

# Source and object files
# Get a list of source files and perform pattern substitution to get object files
SERVER_SRC := $(wildcard server/src/*.c)
SERVER_OBJ := $(SERVER_SRC:server/src/%.c=server/build/%.o)

CLIENT_SRC := $(wildcard client/src/*.c)
CLIENT_OBJ := $(CLIENT_SRC:client/src/%.c=client/build/%.o)

SHARED_SRC := $(wildcard shared/src/*.c)
SHARED_OBJ := $(SHARED_SRC:shared/src/%.c=shared/build/%.o)

# Default rule
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Link object files to create executable
$(SERVER_TARGET): $(SERVER_OBJ) $(SHARED_OBJ)
	$(CC) $(SERVER_OBJ) $(SHARED_OBJ) -o $@

$(CLIENT_TARGET): $(CLIENT_OBJ) $(SHARED_OBJ)
	$(CC) $(CLIENT_OBJ) $(SHARED_OBJ) -o $@

# Build object files
server/build/%.o : server/src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Iserver/include -c $< -o $@

client/build/%.o : client/src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Iclient/include -c $< -o $@

shared/build/%.o : shared/src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf server/build client/build shared/build $(SERVER_TARGET) $(CLIENT_TARGET)

.PHONY: all clean

