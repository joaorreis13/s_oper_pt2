#include "fifo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

//Function that creates FIFO
int create_fifo_request(const char *name)
{
    int fd;
    char fifo[14];
    sprintf(fifo, "/tmp/requests");

    // create the FIFO (named pipe)
    if (mkfifo(fifo, 0666) < 0)
    {
        perror("Creating fifo");
        return -1;
    }
    // Write something to fifo
    fd = open(fifo, O_WRONLY);

    return fd;
}

//FIFO that receives the answer to the client

int create_fifo_anspid(const char *name)
{
    int fd;
    char fifo[14] = "/tmp/";
    sprintf(fifo + 6, "%s", name);
     // create the FIFO (named pipe)
    if (mkfifo(fifo, 0666) < 0)
    {
        perror("Creating fifo");
        return -1;
    }
    fd = open(fifo, O_RDONLY);
    
    return fd;
}

//Function that closes fifo

int close_fifo(const char *fifo, int fd)
{

    if (close(fd) != 0)
    {
        perror("fifo close");
        return -1;
    }
    if (unlink(fifo) != 0)
    {
        perror("fifo close");
        return -1;
    }

    return 0;
}