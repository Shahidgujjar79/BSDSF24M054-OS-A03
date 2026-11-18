CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -Iinclude

TARGET = bin/myshell
SOURCES = src/main.c src/shell.c src/execute.c

all: $(TARGET)

$(TARGET): $(SOURCES) include/shell.h
	mkdir -p bin
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
