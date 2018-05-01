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

#define WIDTH_SEAT 4
#define WIDTH_PID 5

//TO-DOS

//escrita em clog.txt
//escrita em cbook.txt
 
//Function that initializes the client
int client_init(char *time_out, char *num_wanted_seats, char *pref_seat_list)
{

}
//FIFO que recebe os pedidos de reserva de lugar
int create_fifo_request_write()
{	
	//FIFO deve ter o nome ansXXXXX, em que XXXXX representa o PID do cliente (TO-DO)
	int fd;
	char *request = "/tmp/request";

	// create the FIFO (named pipe)
	mkfifo(request, 0666);

	// Write something to fifo
	fd = open(request, O_WRONLY);
	write(fd, "Hi", sizeof("Hi"));
	close(fd);

	// remove the FIFO
	unlink(request);

	return 0;
}

int main(int argc, char *argv[])
{

	client_init(argv[1], argv[2], argv[3]);

	return 0;
}