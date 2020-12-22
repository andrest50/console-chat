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

#define PORT 52500
#define DEBUG 0

//function prototypes
void setupAddressStruct(struct sockaddr_in*, int, char*);
void getUserName(char*);
int checkMessage(int, char*);
void getMessage(char*, char*, int);
void receivedMessage(char*, char*);
void returnMessage(int, char*);
void sendMessage(int, char*);

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

int checkMessage(int socketFD, char* buffer){
    if(strstr(buffer, "$file")){
        char content[10000];
        char file[20];       
        strcpy(file, strtok(buffer, " "));
        strcpy(file, strtok(NULL, " "));
        //printf("%s\n", file);
        if(file != NULL){
            int errNo = 0;
            int* errPtr = &errNo;
            strcpy(content, getFileContents(file, content, errPtr));
            if(errNo == 0){
                printf("%s\n", content);
            }
            char one = '1';
            strncat(content, &one, 1);
            int charsWritten = send(socketFD, content, strlen(content)+1, 0);
        }
        return 1;
    }
    return 0;
}

void getMessage(char* buffer, char* username, int messagesSent){
    buffer[0] = '\0';
    //printf("%s: ", username);
    if(messagesSent == 0)
        fgets(buffer, 256, stdin);
    fgets(buffer, 256, stdin);
    buffer[strlen(buffer)-1] = '\0';
}

void receivedMessage(char* readBuffer, char* username){
    if(!strstr(readBuffer, username)){
        if(strstr(readBuffer, "exit()")){
            char userWhoLeft[20];
            strcpy(userWhoLeft, strtok(readBuffer, ":"));
            printf("%s left.\n", userWhoLeft);
        }
        else {
            if ((fseek(stdin, 0, SEEK_END), ftell(stdin)) > 0){
                fprintf(stdout, "\n%s\n", readBuffer);
            }
            else {
                printf("%s\n", readBuffer);
                fflush(stdout);
            }
        }
        //printf("%s: ", username);
        readBuffer[0] = '\0';
    }
}

void returnMessage(int socketFD, char* buffer){
    int charsRead = recv(socketFD, buffer, 256, 0);
    if(buffer[charsRead-2] == '$'){
        buffer[charsRead-2] = '\0';
        printf("%s\n", buffer);
    }
    //buffer[0] = '\0';
}

void sendMessage(int socketFD, char* buffer){
    if(DEBUG == 1)
        printf("buffer length: %ld\n", strlen(buffer));

    int charsWritten = send(socketFD, buffer, strlen(buffer)+1, 0);
    buffer[0] = '\0';
}

int checkReceivedFileContent(){
    printf("You received file content. Do you want to display it. (1) yes (2) no.\n");
    int resp;
    scanf("%d", &resp);
    return resp;
}

int main(int argc, char *argv[]) {
    int socketFD, portNumber, port = PORT;
    int charsWritten, charsRead, messagesSent = 0, accept;
    struct sockaddr_in serverAddress;
    char buffer[256];
    char readBuffer[256];
    char message[257];
    char context;

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

    //use select() for multiplexing, listening to stdin and socket
    fd_set rfds;
    
    //loop

    do {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(socketFD, &rfds);
        if(select(socketFD + 1, &rfds, NULL, NULL, NULL) == -1){
            perror("select()");
            exit(1);
        }
        if(FD_ISSET(0, &rfds)){
            //printf("here2\n");
            getMessage(buffer, username, messagesSent);
            int type = checkMessage(socketFD, buffer);
            sprintf(message, "%s%d", buffer, type);
            if(type == 0){
                sendMessage(socketFD, message);
                returnMessage(socketFD, message);
            }  
            messagesSent++;
        }
        if(FD_ISSET(socketFD, &rfds)){
            //printf("here\n");
            accept = 1;
            //printf("%s\n", context);
            charsRead = read(socketFD, readBuffer, 256);
            //printf("%s\n", readBuffer);
            if(charsRead == -1){
                perror("read()");
                exit(1);
            }
            context = readBuffer[charsRead-2];
            if(context == 49) //1
                accept = checkReceivedFileContent();
            readBuffer[charsRead-2] = '\0';
            if(accept == 1)
                receivedMessage(readBuffer, username);
        }

    } while(!strstr(buffer, "exit()"));

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