CC=gcc

make: server.c client.c
	$(CC) -o server -pthread server.c
	$(CC) -o client client.c

clean:
	rm -f server client
