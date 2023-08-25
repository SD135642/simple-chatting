CC     = gcc
CDFLAGS = -g -Wall -Werror -pedantic-errors -Iinclude
SRC_DIR = ./src
INCLUDE_DIR = ./include
BUILD_DIR = ./build
CFLAGS = -Wall -Werror -pedantic-errors -Iinclude -O3

.PHONY: all clean debug
all: build
build: $(SRC_DIR)/chat_client.c $(SRC_DIR)/chat_server.c $(SRC_DIR)/util.c
	mkdir -p $(BUILD_DIR)/obj $(BUILD_DIR)/release
	$(CC) $(CFLAGS) $(SRC_DIR)/util.c -c -o $(BUILD_DIR)/obj/util.o
	$(CC) $(CFLAGS) $(SRC_DIR)/chat_client.c $(BUILD_DIR)/obj/util.o -o $(BUILD_DIR)/release/chat_client
	$(CC) $(CFLAGS) $(SRC_DIR)/chat_server.c $(BUILD_DIR)/obj/util.o -o $(BUILD_DIR)/release/chat_server
clean:
	rm -rf $(BUILD_DIR)
debug: $(SRC_DIR)/chat_client.c $(SRC_DIR)/chat_server.c $(SRC_DIR)/util.c
	mkdir -p $(BUILD_DIR)/obj $(BUILD_DIR)/debug
	$(CC) $(CDFLAGS) $(SRC_DIR)/util.c -c -o $(BUILD_DIR)/obj/util.o
	$(CC) $(CDFLAGS) $(SRC_DIR)/chat_client.c $(BUILD_DIR)/obj/util.o -o $(BUILD_DIR)/debug/chat_client
	$(CC) $(CDFLAGS) $(SRC_DIR)/chat_server.c $(BUILD_DIR)/obj/util.o -o $(BUILD_DIR)/debug/chat_server
