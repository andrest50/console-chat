#ifndef USER
#define USER

#include <pthread.h>

//user struct
typedef struct user {
    int userNo;
    char username[20];
    int userSocket;
    pthread_t thread;
    struct user* next;
} User;

extern User* users;
extern User* head;

void removeUser(User*);
void setupUser(User*, int, int);
void addUser(int, int);

#endif