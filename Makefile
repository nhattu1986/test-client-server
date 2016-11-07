CC=gcc

all: client server

client: client.c
	$(CC) -o $@ $^
server: server.c
	$(CC) -o $@ $^

.PHONY: clean

clean:
	rm -f client server *~
