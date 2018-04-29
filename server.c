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

//Defines

#define MAX_ROOM_SEATS = 140
#define MAX_CLI_SEATS = 100
#define WIDTH_SEAT = 4 
//Function that initializes the server
int startserver(char *num_room_seats, char *num_ticket_offices, char *open_time){


}

int main(int agrc,char *argv[]){
    
    startserver(argv[1],argv[2],argv[3]);


    return 0;
}