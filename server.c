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
//struct user* users = NULL;
struct user* users[5];
int connectionsMade = 0;
int port;

//user struct
struct user {
    int userNo;
    char username[20];
    int userSocket;
    //struct user* next;
};

//function prototypes
void setupAddressStruct(struct sockaddr_in*, int);
void displayConnected(struct user*);
int checkMessage(struct user*, char*);
void* connection(void*);
int sendUsersOnline(struct user*, char*);
int sendPortNumber(struct user*);

//set up address struct
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

    //set up connection message
    printf("%s connected.\n", user->username);
    sprintf(connectionsMsg, "Users connected: %d", connectionsMade);
    printf("%s\n", connectionsMsg);
    strcat(connectionsMsg, "0"); //add context

    charsWritten = send(user->userSocket, connectionsMsg, strlen(connectionsMsg), 0); //send connection message

    //send who is online/joined to every user
    for(int i = 0; i < connectionsMade; i++){
        printf("%s is online.\n", users[i]->username);
        
        //loop through connected users
        for(int j = 0; j < connectionsMade; j++){
            strcpy(joinMsg, users[i]->username); //add username to message

            //if user is the one who connected show who is online
            if(users[i]->userNo == j){
                strcat(joinMsg, " is online..");
            }
            else if(user->userNo == j){
                strcat(joinMsg, " is online.0");
            }
            else {
                strcat(joinMsg, " has joined.0");
            }
            charsWritten = send(users[j]->userSocket, joinMsg, strlen(joinMsg), 0); //send user online/joined message
            joinMsg[0] = '\0'; //clear message  
        }
    }
}

int sendUsersOnline(struct user* user, char* message){
    char usersList[100];
    char usersHeader[20];

    //craft message 
    sprintf(usersHeader, "Users online (%d): ", connectionsMade);
    strcpy(usersList, usersHeader);

    //add usernames to message
    for(int i = 0; i < connectionsMade; i++){
        strcat(usersList, users[i]->username);
        if(i != connectionsMade - 1)
            strcat(usersList, ", ");
    }
    strcat(usersList, "$"); //add context
    int charsWritten = send(user->userSocket, usersList, strlen(usersList), 0); //send message to client
    return 1;
}

int sendPortNumber(struct user* user){
    char portString[15];
    sprintf(portString, "%d$", port); //craft message
    int charsWritten = send(user->userSocket, portString, strlen(portString), 0); //send message to client
    return 2;
}

int checkMessage(struct user* user, char* message){
    //check for users command
    if(strcmp(message, "$users") == 0){
        return sendUsersOnline(user, message);
    }
    //check for port command
    if(strcmp(message, "$port") == 0){
        return sendPortNumber(user);
    }
    return 0;
}

void* connection(void* arg){
    //variable declaration/initialization
    struct user* user = arg;
    int charsRead = 0, charsWritten = 0;
    char context;
    char message[256];
    char packaging[256];

    //read username
    charsRead = recv(user->userSocket, user->username, 20, 0);
    printf("Connected with socket fd: %d\n", user->userSocket);
    displayConnected(user);

    //read messages and send to other users
    do {
        message[0] = '\0';
        packaging[0] = '\0';
        charsRead = recv(user->userSocket, message, 256, 0); //get message
        context = message[charsRead-1]; //store context
        message[charsRead-1] = '\0'; //remove context from message

        //if message is not a user exiting, display message for reference
        if(strcmp(message, "exit()") != 0){
            printf("%s: %s\n", user->username, message);
        }

        //if message needs to be sent (commands send themselves)
        if(checkMessage(user, message) == 0){

            //set up message 
            strcpy(packaging, user->username);
            strcat(packaging, ": ");
            strcat(packaging, message);
            int length = strlen(packaging);
            char dot = '.';

            //send message to each user with specific context
            for(int i = 0; i < connectionsMade; i++){
                if(i == user->userNo)
                    strncat(packaging, &dot, 1);
                else
                    strncat(packaging, &context, 1);
                charsWritten = send(users[i]->userSocket, packaging, length+1, 0); //send message
                packaging[length] = '\0'; //clear context
            }     
        }                                      
    } while(strcmp(message, "exit()") != 0);

    printf("%s has left.\n", user->username);
    connectionsMade--; 
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

    port = atoi(argv[1]);
    setupAddressStruct(&serverAddress, port);

    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
        perror("ERROR on binding");
    }

    listen(listenSocket, 5); 

    //server runs indefinitely
    while(1){
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
        if (connectionSocket < 0){
            perror("ERROR on accept");
            exit(1);
        }

        //set up user struct
        users[connectionsMade] = malloc(sizeof(struct user));
        users[connectionsMade]->userSocket = connectionSocket;
        users[connectionsMade]->userNo = connectionsMade;

        //create thread for client and server connection
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