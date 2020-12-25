#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "user.h"

#define EXIT "$e"
#define DEBUG 1

//global variables
struct user* users = NULL; //linked list of all users
struct user* head = NULL; //pointer to head of linked list
int usersOnline = 0, totalConnections = 0;
int port;

//function prototypes
void setupAddressStruct(struct sockaddr_in*, int);
void displayConnected(struct user*);
int checkMessage(struct user*, char*);
void* connection(void*);
int sendUsersOnline(struct user*, char*);
void debugInfo();
char* setupConnectionsMsg(struct user*, char*);
char* sendUsersMsg(struct user*);
int sendFileDescriptor(struct user*);
int sendUserNumber(struct user*);
void disconnect(struct user*);
void deconstructThreads();

//set up address struct
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
  memset((char*) address, '\0', sizeof(*address)); 
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);
  address->sin_addr.s_addr = INADDR_ANY;
}

char* setupConnectionsMsg(struct user* user, char* connectionsMsg){
    printf("%s connected with socket fd: %d\n", user->username, user->userSocket);
    sprintf(connectionsMsg, "Users connected: %d", usersOnline);
    printf("%s\n", connectionsMsg);
    strcat(connectionsMsg, "0"); //add context
    return connectionsMsg;
}

char* sendUsersMsg(struct user* user){
    struct user* localHead = head;
    struct user* localHead2 = head;
    char joinMsg[30];
    int charsWritten;

    for(int i = 0; i < usersOnline; i++){
        printf("%s is online.\n", localHead->username);
        localHead2 = head;
        
        //loop through connected users
        for(int j = 0; j < usersOnline; j++){
            strcpy(joinMsg, localHead->username); //add username to message

            //printf("localHead: %d, localHead2: %d, user: %d\n", localHead->userNo, localHead2->userNo, user->userNo);
            //if user is the one who connected show who is online
            if(localHead2->userNo == localHead->userNo){
                strcat(joinMsg, " is online..");
            }
            else if(user->userNo == localHead->userNo){
                strcat(joinMsg, " has joined.0");
            }
            else if(user->userNo == localHead2->userNo) {
                strcat(joinMsg, " is online.0");
            }
            else {
                strcat(joinMsg, "."); //don't display
            }
            charsWritten = send(localHead2->userSocket, joinMsg, strlen(joinMsg), 0); //send user online/joined message
            localHead2 = localHead2->next;  
        }
        localHead = localHead->next;
    }
}

void displayConnected(struct user* user){
    int charsWritten;
    char connectionsMsg[20];

    //set up connection message
    strcpy(connectionsMsg, setupConnectionsMsg(user, connectionsMsg));

    //send connection message
    charsWritten = send(user->userSocket, connectionsMsg, strlen(connectionsMsg), 0); 

    //traverse();
    //printf("%s\n", localHead->username);

    sendUsersMsg(user); //send who is online/joined to every user
}

int sendUsersOnline(struct user* user, char* message){
    char usersList[100];
    char usersHeader[20];
    struct user* localHead = head;

    //craft message 
    sprintf(usersHeader, "Users online (%d): ", usersOnline);
    strcpy(usersList, usersHeader);

    //add usernames to message
    for(int i = 0; i < usersOnline; i++){
        strcat(usersList, localHead->username);

        //add comma if not last user in list
        if(i != usersOnline - 1)
            strcat(usersList, ", ");

        localHead = localHead->next;
    }
    strcat(usersList, "$"); //add context
    int charsWritten = send(user->userSocket, usersList, strlen(usersList), 0); //send message to client
    return 1;
}

/*send file descriptor message*/
int sendFileDescriptor(struct user* user){
    char fdString[15];
    sprintf(fdString, "Fd: %d$", user->userSocket);
    int charsWritten = send(user->userSocket, fdString, strlen(fdString), 0);
    return 3;
}

/*send user number message*/
int sendUserNumber(struct user* user){
    char numberString[20];
    sprintf(numberString, "User number: %d$", user->userNo);
    int charsWritten = send(user->userSocket, numberString, strlen(numberString), 0);
    return 4;
}

void debugInfo(){
    struct user* localHead = head;
    int count = 1;
    while(localHead != NULL){
        printf("-------------------------------------\n");
        printf("==Node %d==\n", count);
        printf("username: %s\n", localHead->username);
        printf("number: %d\n", localHead->userNo);
        printf("socket: %d\n", localHead->userSocket);
        printf("thread: %ld\n", pthread_self());
        localHead = localHead->next;
        count++;
    }
    printf("-------------------------------------\n");
}

/*Check messages for a command starting with $*/
int checkMessage(struct user* user, char* message){
    if(strcmp(message, "$users") == 0){
        return sendUsersOnline(user, message);
    }
    if(strcmp(message, "$fd") == 0){
        return sendFileDescriptor(user);
    }
    if(strcmp(message, "$number") == 0){
        return sendUserNumber(user);
    }
    return 0;
}

void sendMessage(struct user* user, char* message, char context){
    struct user* localHead = head; //head of linked list
    char packaging[1000];
    char dot = '.';
    int charsWritten;

    //set up message 
    strcpy(packaging, user->username);
    strcat(packaging, ": ");
    strcat(packaging, message);

    //send message to each user with specific context
    localHead = head; //set pointer back to head to traverse linked list
    for(int i = 0; i < usersOnline; i++){

        //if this is the user sending the message
        if(i == user->userNo)
            strncat(packaging, &dot, 1);
        else
            strncat(packaging, &context, 1);

        charsWritten = send(localHead->userSocket, packaging, strlen(packaging), 0); //send message
        packaging[strlen(packaging)-1] = '\0'; //clear context
        localHead = localHead->next;
    } 
}

void disconnect(struct user* user){
    printf("%s has left.\n", user->username);
    removeUser(user); //remove user from linked list
    usersOnline--; //decrease number of users online
    close(user->userSocket);
}

void* connection(void* arg){
    
    //variable declaration/initialization
    struct user* user = arg; //user for this connection
    int charsRead = 0, charsWritten = 0;
    char context;
    char message[1000];

    //read username
    charsRead = recv(user->userSocket, user->username, 20, 0);
    displayConnected(user);

    //read messages and send to other users
    do {
        charsRead = recv(user->userSocket, message, 1000, 0); //get message
        if(charsRead == 0 || errno == EINTR){
            charsRead = 3;
            strcpy(message, "$e0");
        }
        context = message[charsRead-1]; //store context
        message[charsRead-1] = '\0'; //remove context from message

        //if message is not a user exiting, display message
        if(strcmp(message, EXIT) != 0){
            printf("%s: %s\n", user->username, message);
        }

        //if message needs to be sent (commands send themselves)
        if(checkMessage(user, message) == 0){
            sendMessage(user, message, context);    
        }                                      
    } while(strcmp(message, EXIT) != 0);

    disconnect(user);
}

void deconstructThreads(){
    struct user* localHead = head;
    for(int i = 0; i < usersOnline; i++){
        pthread_join(localHead->thread, NULL);
        localHead = localHead->next;
    }
}

int main(int argc, char* argv[]){
    int connectionSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    //signal(SIGPIPE,SIG_IGN);

    if (argc < 2) { 
        fprintf(stderr,"USAGE: %s port\n", argv[0]); 
        exit(1);
    } 
    port = atoi(argv[1]);

    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        perror("ERROR opening socket");
    }

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

        //add user to linked list
        addUser(connectionSocket, totalConnections);

        //create thread for client and server connection
        pthread_create(&users->thread, NULL, connection, (void*) users);

        usersOnline++;
        totalConnections++;
    }

    deconstructThreads();

    close(listenSocket); 

    return 0;
}