#ifndef USER
#define USER

#include <pthread.h>

//user struct
struct user {
    int userNo;
    char username[20];
    int userSocket;
    pthread_t thread;
    struct user* next;
};

extern struct user* users;
extern struct user* head;

void removeUser(struct user*);
void setupUser(struct user*, int, int);
void addUser(int, int);

#endif