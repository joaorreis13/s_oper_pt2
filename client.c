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

#define WIDTH_SEAT 4
#define WIDTH_PID 5
#define MAX_TIME_OUT 20
#define MAX_SEATS 10
#define MAX_BUF 1024
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

int create_fifo_resposta_read()
{
	int fd;
	char *resposta = "/tmp/resposta";
	char buf[MAX_BUF];

	// open, read, and display the message from the FIFO
	fd = open(resposta, O_RDONLY);
	read(fd, buf, MAX_BUF);
	printf("Received: %s\n", buf);
	close(fd);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		printf("Use: %s <time_out> <num_wanted_seats> <pref_seat_list>\n", *argv);
		return 0;
	}

	unsigned int time_out,num_wanted_seats;
	char pref_seat_list[30];

	if (time_out > MAX_TIME_OUT)
		printf("Timeout too big\n");

	if (num_wanted_seats > MAX_SEATS)
		printf("Too much wanted seats\n");


	return 0;
}