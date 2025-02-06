CC = gcc
CFLAGS = -Wall -Wextra -I./include $(shell pkg-config --cflags x11 libnotify json-c libcurl)
LIBS = $(shell pkg-config --libs x11 libnotify json-c libcurl) -pthread

SRC_DIR = src
BUILD_DIR = build
TARGET = app_tracker

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)