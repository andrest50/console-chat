#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

//global variables
pthread_t threads[5];
//int connectionSockets[5];
//char usernames[5][20];
struct user* users[5];
int connectionsMade = 0;

struct user {
    int userNo;
    char username[20];
    int userSocket;
};

void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  memset((char*) address, '\0', sizeof(*address)); 
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);
  address->sin_addr.s_addr = INADDR_ANY;
}

void displayConnected(struct user* user){
    printf("%s connected.\n", user->username);
    printf("Users connected: %d\n", connectionsMade);
    for(int i = 0; i < connectionsMade; i++){
        printf("%s is online.\n", users[i]->username);
    }
}

void* connection(void* arg){
    struct user* user = arg;
    int charsRead = 0, charsWritten = 0;
    char message[256];
    char packaging[256];

    //read username
    charsRead = recv(user->userSocket, user->username, 20, 0);
    printf("Connected with socket fd: %d\n", user->userSocket);
    displayConnected(user);

    //read messages and send to other users
    do {
        message[0] = '\0';
        //buffer[0] = '\0';
        charsRead = recv(user->userSocket, message, 256, 0);
        if(strcmp(message, "exit()") != 0){
            printf("%s: %s\n", user->username, message);
        }
        strcpy(packaging, user->username);
        strcat(packaging, ": ");
        strcat(packaging, message);
        for(int i = 0; i < connectionsMade; i++){
            charsWritten = send(users[i]->userSocket, packaging, strlen(packaging)+1, 0);
        }                                           
    } while(strcmp(message, "exit()") != 0);

    printf("%s has left.\n", user->username); 
    close(user->userSocket);
}

int main(int argc, char* argv[]){
    int connectionSocket;
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

        users[connectionsMade] = malloc(sizeof(struct user));
        users[connectionsMade]->userSocket = connectionSocket;
        users[connectionsMade]->userNo = connectionsMade;

        pthread_create(&threads[connectionsMade], NULL, connection, (void*) users[connectionsMade]);

        connectionsMade++;
    }

    for(int i = 0; i < connectionsMade; i++){
        pthread_join(threads[i], NULL);
    }

    close(listenSocket); 

    return 0;
}

//For larger messages

/*while(strcmp(buffer, "@@") != 0){
    buffer[0] = '\0';
    charsRead = recv(connectionSocket, buffer, 256, 0);
    if(strcmp(buffer, "@@") != 0){
        strcat(fullBuffer, buffer);
    }     
}*/