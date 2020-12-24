#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>    
#include <poll.h>
#include <sys/select.h>
#include "commands.h"

#define PORT 52500 //default port
#define DEBUG 0
#define EXIT "$e"

int socketFD;
int port = PORT;

//function prototypes
void setupAddressStruct(struct sockaddr_in*, int, char*);
void getUserName(char*);
int checkMessage(char*);
void getMessage(char*, char*, int);
void receivedMessage(char*, char, char*);
void returnMessage(char*);
void sendMessage(char*);
int checkReceivedFileContent();
int userInput(char*, int*);
void serverInput(char*);

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
    //get username until it is valid
    do {
        printf("Enter a username: ");
        scanf("%s", username);
    } while(strlen(username) > 20);
    printf("Username set to %s\n", username);
}

int checkMessage(char* buffer){

    //if user is sending a file
    if(strstr(buffer, "$file")){
        char content[10000]; //contains file content
        char file[20]; //contains file name
        strcpy(file, strtok(buffer, " "));
        strcpy(file, strtok(NULL, " ")); //get file name as second argument to $file

        //if file was specified        
        if(file != NULL){
            int errNo = 0; //indicates errors
            int* errPtr = &errNo;
            strcpy(content, getFileContents(file, content, errPtr)); //get file content
            if(errNo == 0){
                printf("%s\n", content);
            }
            content[strlen(content)] = '1'; //append the content
            int charsWritten = send(socketFD, content, strlen(content), 0); //send to server
        }
        return 1;
    }
    return 0;
}

void getMessage(char* buffer, char* username, int messagesSent){
    buffer[0] = '\0'; //clear buffer

    //if first message, use extra fgets to avoid blank line
    if(messagesSent == 0)
        fgets(buffer, 256, stdin);

    fgets(buffer, 256, stdin); //get user input
    buffer[strlen(buffer)-1] = '\0'; //remove newline
}

void receivedMessage(char* readBuffer, char context, char* username){

    //if message is to be displayed
    if(context != '.'){

        //if message is someone leaving chat
        if(strstr(readBuffer, EXIT)){
            char userWhoLeft[20];
            strcpy(userWhoLeft, strtok(readBuffer, ":"));
            printf("%s left.\n", userWhoLeft);
        }
        else {
            //if there is user input
            if ((fseek(stdin, 0, SEEK_END), ftell(stdin)) > 0){
                fprintf(stdout, "\n%s\n", readBuffer);
            }
            else {
                printf("%s\n", readBuffer);
                fflush(stdout);
            }
        }
    }
    readBuffer[0] = '\0'; //clear message
}

void returnMessage(char* message){
    int charsRead = recv(socketFD, message, 256, 0); //receive a response from server
    
    //if context is $ then print response (e.g. $port, $users)
    if(message[charsRead-1] == '$'){
        message[charsRead-1] = '\0';
        printf("%s\n", message);
    }
}

void sendMessage(char* message){
    //print string length if on debug mode
    if(DEBUG == 1)
        printf("message length: %ld\n", strlen(message));

    int charsWritten = send(socketFD, message, strlen(message), 0); //send message to server
    message[0] = '\0'; //clear message
}

int checkReceivedFileContent(){
    //prompt user to display file content received
    printf("You received file content. Do you want to display it. (1) yes (2) no.\n");
    int resp;
    scanf("%d%*c", &resp); //get integer & throw out newline
    //fflush(stdout);
    return resp;
}

int userInput(char* username, int* messagesSent){
    char buffer[256];
    char message[257];

    getMessage(buffer, username, *messagesSent); //get message
    int type = checkMessage(buffer); //check message for commands
    sprintf(message, "%s%d", buffer, type); //combine message + context

    //if message is to be sent to server
    if(type == 0){
        sendMessage(message); //send message to server
        returnMessage(message); //return response from server
    }  
    (*messagesSent)++;
    if(strstr(buffer, EXIT) != NULL)
        return 1;
    return 0;
}

void serverInput(char* username){
    char readBuffer[256];
    int accept = 1;
    
    int charsRead = read(socketFD, readBuffer, 256); //read from socket
    if(charsRead == -1){
        perror("read()");
        exit(1);
    }
    char context = readBuffer[charsRead-1]; //store context

    //check if context is a 1 to prompt for response
    if(context == 49) 
        accept = checkReceivedFileContent(); //user decides to accept content
    readBuffer[charsRead-1] = '\0';
    
    //if user allows content to be displayed
    if(accept == 1)
        receivedMessage(readBuffer, context, username);
}

int main(int argc, char *argv[]) {

    //variable declaration/initialization
    int charsWritten, charsRead, messagesSent = 0;
    struct sockaddr_in serverAddress;
    char username[20];

    if (argc < 2) { 
        fprintf(stderr,"USAGE: %s port\n", argv[0]); 
        exit(1); 
    }
    port = atoi(argv[1]);

    socketFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (socketFD < 0){
        perror("CLIENT: ERROR opening socket");
        exit(1);
    }

    setupAddressStruct(&serverAddress, port, "localhost");
    
    getUserName(username); //get username

    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("CLIENT: ERROR connecting");
        exit(1);
    }

    printf("Succesfully connected to server on port %d\n", port);

    //printf("[%ld] %s\n", strlen(username), username);
    charsWritten = send(socketFD, username, strlen(username)+1, 0); //send username to server
    if (charsWritten < 0){
        perror("CLIENT: ERROR writing to socket");
        exit(1);
    } 

    //use select() for multiplexing, listening to stdin and socket
    fd_set rfds;
    
    //loop until user leaves
    while(1) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(socketFD, &rfds);

        //use select for i/o multiplexing -- allowing input from socket and stdin somewhat concurrently
        if(select(socketFD + 1, &rfds, NULL, NULL, NULL) == -1){
            perror("select()");
            exit(1);
        }
        //if there is user input in stdin
        if(FD_ISSET(0, &rfds)){
            if(userInput(username, &messagesSent) == 1)
                break;
        }
        //if there is a message from socket
        if(FD_ISSET(socketFD, &rfds)){
            serverInput(username);
        }

    };

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

//Previous code using processes

 /*pid_t spawnid;
    spawnid = fork();
    switch(spawnid){
        case -1:
            perror("fork() failed");
            exit(1);
        case 0:
            while(1){
                charsRead = recv(socketFD, readBuffer, 256, 0);
                //printf("here\n");
                if(charsRead == -1){
                    perror("recv()");
                    exit(1);
                }
                receivedMessage(readBuffer, username);
            }
            exit(0);
            break;
        default:
            ;
            int childStatus;
            pid_t childPid = waitpid(spawnid, &childStatus, WNOHANG);
            do {
                getMessage(buffer, username, messagesSent);
                
                totalCharsWritten = 0;
                if(DEBUG == 1)
                    printf("buffer length: %ld\n", strlen(buffer));
                charsWritten = send(socketFD, buffer, strlen(buffer)+1, 0);
                messagesSent++;

                //buffer[0] = '\0';
                //charsRead = recv(socketFD, buffer, 256, 0);
                //if(DEBUG == 1)
                    //printf("[%d]%s\n", charsRead, buffer);
            } while(!strstr(buffer, "exit()"));
            
            kill(childPid, SIGKILL);
            break;
    }*/