#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include "commands.h"

char* getFileContents(char* filename, char* content, int* errNo){
    int fd = open(filename, O_RDONLY); //open file for read only
    if(fd == -1){
        perror("open()");
        *errNo = 2;
    }
    long fsize = lseek(fd, 0, SEEK_END); //get number of characters in file
    lseek(fd, 0, SEEK_SET); //reset position to start of file

    //read entire file
    if(read(fd, content, fsize) == -1){
        perror("read()");
        *errNo = 2;
    }
    return content; //return file content
}