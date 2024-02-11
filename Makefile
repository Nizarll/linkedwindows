CC=gcc
CFLAGS=-Wall -Wextra -std=c11
LDFLAGS=-lraylib -lpthread -ldl -lrt -lX11 -lm

.PHONY: all clean

all: client server

client: client.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server: server.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f client server
