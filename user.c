#include <stdlib.h>
#include <stdio.h>
#include "user.h"

/*Disconnect user by removing node from linked list*/
void removeUser(User* user){
    User* localHead = head;
    User* prev = head;

    //if node to delete is head node
    if(localHead != NULL && localHead->userNo == user->userNo){
        head = localHead->next;
        free(localHead);
        return;
    }

    //traverse until node to delete is found
    while(localHead != NULL && localHead->userNo != user->userNo){
        prev = localHead;
        localHead = localHead->next;
    }

    //if node to delete doesn't exist
    if(localHead == NULL) 
        return;

    //skip the node to delete (break from link)
    prev->next = localHead->next;

    //debugInfo();
    free(localHead);
}

void setupUser(User* newUser, int connectionSocket, int totalConnections){
    newUser->userSocket = connectionSocket;
    newUser->userNo = totalConnections;
    newUser->next = NULL;
}

void addUser(int connectionSocket, int totalConnections){

    //initialize struct
    User* newUser = (User*) malloc(sizeof(User));
    setupUser(newUser, connectionSocket, totalConnections);

    //if this is the first user initialize linked list
    if(head == NULL){
        users = newUser;
        head = users; //set head as first user
    }
    //add another user to te linked list
    else {
        users = head;
        while(users->next != NULL){
            users = users->next;
        }
        users->next = newUser;
        users = users->next; //point to next user
    }
}