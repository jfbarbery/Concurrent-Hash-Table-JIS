CC = gcc
CFLAGS = -Wall -pthread -g
TARGET = chash

all: $(TARGET)

$(TARGET): chash.c
	$(CC) $(CFLAGS) chash.c -o $(TARGET)

clean:
	rm -f $(TARGET) output.txt
