CC=gcc
OBJ=server.c client.c user.c commands.c
EXE=server client

all: $(EXE)

server: $(OBJ)
	$(CC) -o $@ -pthread server.c user.c
	
client: $(OBJ)
	$(CC) -o $@ client.c commands.c

clean:
	rm -f *.o $(EXE)
