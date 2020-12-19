#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  memset((char*) address, '\0', sizeof(*address)); 
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);
  address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char* argv[]){
    int connectionSocket, charsRead, connectionsMade = 0;
    char* usernames[20];
    char buffer[256];
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    if (argc < 2) { 
        fprintf(stderr,"USAGE: %s port\n", argv[0]); 
        exit(1);
    } 
    
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        perror("ERROR opening socket");
    }

    setupAddressStruct(&serverAddress, atoi(argv[1]));

    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
        perror("ERROR on binding");
    }

    listen(listenSocket, 5); 

    while(1){
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
        
        if (connectionSocket < 0){
            perror("ERROR on accept");
            exit(1);
        }

        connectionsMade++;

        int charsRead = recv(connectionSocket, usernames[connectionsMade-1], 20, 0);
        printf("%s connected.\n", usernames[connectionsMade-1]);

        close(connectionSocket); 
    }

    close(listenSocket); 

    return 0;
}