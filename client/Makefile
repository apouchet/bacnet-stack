# BACnet Client Makefile
# Build the bacnet-client CLI and library

# Configuration
PREFIX ?=
CC = $(PREFIX)gcc
AR = $(PREFIX)ar
RANLIB = $(PREFIX)ranlib

# Directories
CLIENT_DIR = $(realpath .)
BACNET_SRC_DIR = $(realpath ../src)
BACNET_PORT_DIR = $(realpath ../ports/linux)
BACNET_POSIX_DIR = $(realpath ../ports/posix)
APPS_LIB_DIR = $(realpath ../apps/lib)
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object

# Output
BIN_DIR = $(CLIENT_DIR)/bin
LIB_DIR = $(CLIENT_DIR)/lib
BUILD_DIR = $(CLIENT_DIR)/build

# Targets
CLIENT_BIN = $(BIN_DIR)/bacnet-client
CLIENT_LIB = $(LIB_DIR)/libbacnet_client.a

# BACnet library from apps
BACNET_LIB = $(APPS_LIB_DIR)/libbacnet.a

# BACnet object files needed for client apps
BACNET_OBJECTS = \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c

# Compiler flags
CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -Wno-sign-conversion -Wno-conversion -Wno-sign-compare
CFLAGS += -Wno-long-long -Wno-attributes -Wno-implicit-fallthrough
CFLAGS += -std=gnu99
CFLAGS += -I$(BACNET_SRC_DIR) -I$(BACNET_PORT_DIR) -I$(BACNET_POSIX_DIR)
CFLAGS += -I$(CLIENT_DIR)

# Defines
CFLAGS += -DBACDL_BIP=1
CFLAGS += -DBBMD_ENABLED=1 -DBBMD_CLIENT_ENABLED
CFLAGS += -DWEAK_FUNC=
CFLAGS += -DPRINT_ENABLED=1
CFLAGS += -DBACAPP_ALL
CFLAGS += -DBACNET_TIME_MASTER
CFLAGS += -DBACNET_PROPERTY_LISTS=1
CFLAGS += -DBACNET_PROTOCOL_REVISION=24
CFLAGS += -DBACNET_STACK_DEPRECATED_DISABLE

# Build type
ifeq ($(BUILD),debug)
CFLAGS += -g -O0 -DDEBUG_ENABLED=1
else
CFLAGS += -Os
endif

# Linker flags
LDFLAGS += -L$(APPS_LIB_DIR) -lbacnet
LDFLAGS += -lpthread -lm -lrt

# Sources
CLIENT_LIB_SRCS = $(LIB_DIR)/bacnet_client.c
CLIENT_BIN_SRCS = $(BIN_DIR)/main.c

# Objects
CLIENT_LIB_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(CLIENT_LIB_SRCS)))
CLIENT_BIN_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(CLIENT_BIN_SRCS)))
BACNET_OBJ_OBJS = $(BUILD_DIR)/device-client.o $(BUILD_DIR)/netport.o

# Default target
.PHONY: all
all: $(BACNET_LIB) $(CLIENT_LIB) $(CLIENT_BIN)

# Build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# BACnet library
$(BACNET_LIB):
	$(MAKE) -C ../apps lib

# Client library
$(CLIENT_LIB): $(BUILD_DIR) $(CLIENT_LIB_OBJS)
	$(AR) rcs $@ $(CLIENT_LIB_OBJS)
	$(RANLIB) $@

# Client binary
$(CLIENT_BIN): $(BUILD_DIR) $(CLIENT_BIN_OBJS) $(CLIENT_LIB) $(BACNET_OBJ_OBJS) $(BACNET_LIB)
	$(CC) $(CLIENT_BIN_OBJS) $(CLIENT_LIB) $(BACNET_OBJ_OBJS) $(LDFLAGS) -o $@
	@echo "Built: $@"

# Compile library sources
$(BUILD_DIR)/bacnet_client.o: $(LIB_DIR)/bacnet_client.c $(LIB_DIR)/bacnet_client.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile binary sources
$(BUILD_DIR)/main.o: $(BIN_DIR)/main.c $(LIB_DIR)/bacnet_client.h
	$(CC) $(CFLAGS) -c $< -o $@

# Compile BACnet object sources
$(BUILD_DIR)/device-client.o: $(BACNET_OBJECT_DIR)/client/device-client.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/netport.o: $(BACNET_OBJECT_DIR)/netport.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(CLIENT_BIN) $(CLIENT_LIB)

# Install
.PHONY: install
install: $(CLIENT_BIN)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(CLIENT_BIN) $(DESTDIR)/usr/local/bin/
	install -d $(DESTDIR)/usr/local/lib
	install -m 644 $(CLIENT_LIB) $(DESTDIR)/usr/local/lib/
	install -d $(DESTDIR)/usr/local/include/bacnet
	install -m 644 $(LIB_DIR)/bacnet_client.h $(DESTDIR)/usr/local/include/bacnet/

# Test
.PHONY: test
test: $(CLIENT_BIN)
	$(MAKE) -C tests

# Help
.PHONY: help
help:
	@echo "BACnet Client Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build everything (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Install to system"
	@echo "  test     - Run tests"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD=debug  - Build with debug symbols"
	@echo "  PREFIX=      - Cross-compile prefix"
