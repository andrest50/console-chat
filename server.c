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
    int charsWritten;
    char joinMsg[30];
    char connectionsMsg[20];

    printf("%s connected.\n", user->username);
    sprintf(connectionsMsg, "Users connected: %d", connectionsMade);
    printf("%s\n", connectionsMsg);
    for(int i = 0; i < connectionsMade; i++){
        printf("%s is online.\n", users[i]->username);
        for(int j = 0; j < connectionsMade; j++){
            strcpy(joinMsg, users[i]->username);
            if(user->userNo == j){
                strcat(joinMsg, " is online.");
            }
            else {
                strcat(joinMsg, " has joined.");
            }
            //charsWritten = send(users[j]->userSocket, connectionsMsg, strlen(connectionsMsg)+1, 0);
            charsWritten = send(users[j]->userSocket, joinMsg, strlen(joinMsg)+1, 0);
            joinMsg[0] = '\0';  
        }
        if(i == user->userNo){
            charsWritten = send(users[i]->userSocket, connectionsMsg, strlen(connectionsMsg)+1, 0);
            connectionsMsg[0] = '\0'; 
        } 
    }
}

int checkMessage(struct user* user, char* message){
    if(strcmp(message, "$users") == 0){
        char usersList[100];
        strcpy(usersList, "Users online: ");
        for(int i = 0; i < connectionsMade; i++){
            strcat(usersList, users[i]->username);
            if(i != connectionsMade - 1)
                strcat(usersList, ", ");
        }
        printf("%s\n", usersList);
        int charsWritten = send(user->userSocket, "$", 2, 0);
        charsWritten = send(user->userSocket, usersList, strlen(usersList)+1, 0);
        return 1;
    }
    return 0;
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
        if(checkMessage(user, message) == 0){
            strcpy(packaging, user->username);
            strcat(packaging, ": ");
            strcat(packaging, message);
            for(int i = 0; i < connectionsMade; i++){
                if(i == user->userNo)
                    charsWritten = send(user->userSocket, ".", 2, 0);
                charsWritten = send(users[i]->userSocket, packaging, strlen(packaging)+1, 0);
            }     
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