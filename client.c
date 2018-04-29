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

#define WIDTH_SEAT = 4
#define WIDTH_PID = 5

//Function that initializes the client
int client_init(char *time_out, char *num_wanted_seats, char *pref_seat_list){


}

int main(int agrc,char *argv[]){
    
    client_init(argv[1],argv[2],argv[3]);


    return 0;
}