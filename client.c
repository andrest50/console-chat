#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netdb.h>     

#define PORT 52500
#define DEBUG 0

void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname){
 
  memset((char*) address, '\0', sizeof(*address)); 

  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);

  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }

  memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

void getUserName(char* username){
    do {
        printf("Enter a username: ");
        scanf("%s", username);
    } while(strlen(username) > 20);
    printf("Username set to %s\n", username);
}

void getMessage(char* buffer, char* username, int messagesSent){
    buffer[0] = '\0';
    printf("%s: ", username);
    if(messagesSent == 0)
        fgets(buffer, 256, stdin);
    fgets(buffer, 256, stdin);
    buffer[strlen(buffer)-1] = '\0';
}

int main(int argc, char *argv[]) {
    int socketFD, portNumber, port = PORT;
    int charsWritten, totalCharsWritten, charsRead, messagesSent = 0;
    struct sockaddr_in serverAddress;
    char buffer[256];

    if (argc < 2) { 
        fprintf(stderr,"USAGE: %s port\n", argv[0]); 
        exit(0); 
    }
    if(argc > 2){
        port = atoi(argv[1]);
    }

    socketFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (socketFD < 0){
        perror("CLIENT: ERROR opening socket");
        exit(1);
    }

    setupAddressStruct(&serverAddress, port, "localhost");

    char username[20];    
    getUserName(username);

    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("CLIENT: ERROR connecting");
        exit(1);
    }

    printf("Succesfully connected to server on port %d\n", port);

    charsWritten = send(socketFD, username, strlen(username)+1, 0);
    if (charsWritten < 0){
        perror("CLIENT: ERROR writing to socket");
        exit(1);
    } 

    do {
        getMessage(buffer, username, messagesSent);
        
        totalCharsWritten = 0;
        if(DEBUG == 1)
            printf("buffer length: %ld\n", strlen(buffer));
        charsWritten = send(socketFD, buffer, strlen(buffer)+1, 0);

        buffer[0] = '\0';
        charsRead = recv(socketFD, buffer, 256, 0);
        if(DEBUG == 1)
            printf("[%d]:%s\n", charsRead, buffer);
        messagesSent++;
    } while(strcmp(buffer, "exit()") != 0);

    close(socketFD); 

  return 0;
}

//For larger messages

/*while(totalCharsWritten < strlen(buffer)){
        charsWritten = send(socketFD, buffer + totalCharsWritten, strlen(buffer), 0);
        printf("chars written: %d\n", charsWritten);
        totalCharsWritten += charsWritten;
}*/
//charsWritten = send(socketFD, "@@", 3, 0);