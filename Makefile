CC=gcc

make: server.c client.c
	$(CC) -o server -pthread server.c
	$(CC) -o client client.c commands.c

clean:
	rm -f server client
