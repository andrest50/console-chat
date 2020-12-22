#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include "commands.h"

char* getFileContents(char* filename, char* content, int* errNo){
    int fd = open(filename, O_RDONLY);
    if(fd == -1){
        perror("open()");
        *errNo = 2;
    }
    long fsize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if(read(fd, content, fsize) == -1){
        perror("read()");
        *errNo = 2;
    }
    return content;
}