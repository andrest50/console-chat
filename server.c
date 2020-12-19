#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  memset((char*) address, '\0', sizeof(*address)); 
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);
  address->sin_addr.s_addr = INADDR_ANY;
}

void checkBgProcesses(int* processes, int connectionsMade){
    for(int i = 0; i < connectionsMade; i++){
        int childStatus = 1;
        pid_t childPid = waitpid(processes[connectionsMade-1], &childStatus, WNOHANG);
        if (WIFEXITED(childStatus)) {
            printf("background pid %d is done : exit value %d\n", processes[i], WEXITSTATUS(childStatus));
            processes[i] = 0;
        }
        else if(WIFSIGNALED(childStatus) == SIGKILL){
            printf("background pid %d is done: terminated by signal %d\n", processes[i], WTERMSIG(childStatus));
            processes[i] = 0;
        }
        else if (WIFSIGNALED(childStatus) == SIGTERM || kill(processes[i], 0) == -1) {
            printf("background pid %d is done: terminated by signal %d\n", processes[i], WTERMSIG(childStatus));
            processes[i] = 0;
        }
    }
}

void displayConnected(char usernames[5][20], int connectionsMade){
    printf("%s connected.\n", usernames[connectionsMade]);
    printf("Users connected: %d\n", connectionsMade + 1);
    for(int i = 0; i <= connectionsMade; i++){
        printf("%s is online.\n", usernames[i]);
    }
}

int main(int argc, char* argv[]){
    int connectionSocket, charsRead, connectionsMade = 0;
    char usernames[5][20];
    int processes[5];
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

        int pipeFDs[2];
        if(pipe(pipeFDs) == -1){
            perror("Call to pipe() failed");
            exit(1);
        }

        pid_t spawnid = -5;
        spawnid = fork();

        switch(spawnid){
            case -1:
                perror("fork() failed");
                exit(1);
            case 0:
                ;
                //char buffer[256];
                char fullBuffer[256];

                close(pipeFDs[0]);

                int charsRead = recv(connectionSocket, usernames[connectionsMade], 20, 0);
                write(pipeFDs[1], usernames[connectionsMade], 20);
                displayConnected(usernames, connectionsMade);

                int charsWritten = 0;
                do {
                    fullBuffer[0] = '\0';
                    //buffer[0] = '\0';
                    charsRead = recv(connectionSocket, fullBuffer, 256, 0);
                    if(strcmp(fullBuffer, "exit()") != 0){
                        printf("%s: %s\n", usernames[connectionsMade], fullBuffer);
                    }                       
                    charsWritten = send(connectionSocket, fullBuffer, strlen(fullBuffer)+1, 0);
                } while(strcmp(fullBuffer, "exit()") != 0);

                printf("%s has left.\n", usernames[connectionsMade]); 
                close(connectionSocket);
                exit(0);

                break;
            default:
                ;
                close(pipeFDs[1]);
                charsRead = read(pipeFDs[0], usernames[connectionsMade], 20);
                pid_t childPid = waitpid(spawnid, &processes[connectionsMade], WNOHANG);
                processes[connectionsMade] = spawnid;
                close(connectionSocket);
                break;
        }

        connectionsMade++;

        checkBgProcesses(processes, connectionsMade);

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