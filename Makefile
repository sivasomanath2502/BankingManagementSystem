# ==========================================
# Banking Management System - Makefile
# ==========================================

CC = gcc
CFLAGS = -Wall -Iinclude
BUILD_DIR = build
SRC_DIR = src
SERVER_DIR = server
CLIENT_DIR = client

# object files
OBJS = $(BUILD_DIR)/bank_ops.o

# targets
SERVER = $(BUILD_DIR)/server
CLIENT = $(BUILD_DIR)/client

# default target
all: $(SERVER) $(CLIENT)

# compile bank_ops.c to object file
$(BUILD_DIR)/bank_ops.o: $(SRC_DIR)/bank_ops.c include/bank_ops.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(SRC_DIR)/bank_ops.c -o $(BUILD_DIR)/bank_ops.o

# build server
$(SERVER): $(SERVER_DIR)/server.c $(OBJS)
	$(CC) $(CFLAGS) $(SERVER_DIR)/server.c $(OBJS) -lpthread -o $(SERVER)
	@echo "✅ Server built successfully!"

# build client
$(CLIENT): $(CLIENT_DIR)/client.c
	$(CC) $(CFLAGS) $(CLIENT_DIR)/client.c -o $(CLIENT)
	@echo "✅ Client built successfully!"

# clean build files
clean:
	rm -rf $(BUILD_DIR)/*.o $(SERVER) $(CLIENT)
	@echo "🧹 Cleaned build directory."

# run server
run-server: $(SERVER)
	./$(SERVER)

# run client
run-client: $(CLIENT)
	./$(CLIENT)

.PHONY: all clean run-server run-client

