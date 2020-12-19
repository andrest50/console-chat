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

        pid_t spawnid = -5;
        spawnid = fork();

        switch(spawnid){
            case -1:
                perror("fork() failed");
                exit(1);
            case 0:
                ;
                char buffer[256];

                connectionsMade++;

                int charsRead = recv(connectionSocket, usernames[connectionsMade-1], 20, 0);
                printf("%s connected.\n", usernames[connectionsMade-1]);
                printf("%d\n", connectionsMade);
                for(int i = 0; i < connectionsMade; i++){
                    printf("%s is online.\n", usernames[i]);
                }

                do {
                    charsRead = recv(connectionSocket, buffer, 256, 0);
                } while(strcmp(buffer, "exit()") != 0);

                printf("%s has left.\n", usernames[connectionsMade-1]); 
                close(connectionSocket);
                exit(0);

                break;
            default:
                ;
                pid_t childPid = waitpid(spawnid, &processes[connectionsMade-1], WNOHANG);
                close(connectionSocket);
                break;
        }

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

    close(listenSocket); 

    return 0;
}