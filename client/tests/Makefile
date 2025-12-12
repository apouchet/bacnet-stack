# BACnet Client Tests Makefile

# Directories
TEST_DIR = $(realpath .)
CLIENT_DIR = $(realpath ..)
BACNET_SRC_DIR = $(realpath ../../src)
BACNET_PORT_DIR = $(realpath ../../ports/linux)
BACNET_POSIX_DIR = $(realpath ../../ports/posix)

# Compiler
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g
CFLAGS += -I$(BACNET_SRC_DIR) -I$(BACNET_PORT_DIR) -I$(BACNET_POSIX_DIR)
CFLAGS += -I$(CLIENT_DIR)
CFLAGS += -DBACDL_BIP=1 -DTEST_ENABLED=1

# Test executable
TEST_BIN = test_client

# Sources
TEST_SRCS = test_main.c

# Objects
TEST_OBJS = $(TEST_SRCS:.c=.o)

# Default target
.PHONY: all
all: $(TEST_BIN)

$(TEST_BIN): $(TEST_OBJS)
	$(CC) $(TEST_OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run
run: $(TEST_BIN)
	./$(TEST_BIN)

.PHONY: clean
clean:
	rm -f $(TEST_OBJS) $(TEST_BIN)

.PHONY: integration
integration:
	./run_integration.sh
