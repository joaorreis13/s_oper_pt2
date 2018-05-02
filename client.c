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
#define MAX_SEATS 99
#define MAX_BUF 1024
//TO-DOS

//escrita em clog.txt
//escrita em cbook.txt
//criar metodo de leitura do argumento <pref_seat_list>

//Function that initializes the client
int client_init(char *time_out, char *num_wanted_seats, char *pref_seat_list)
{
}
//FIFO that sends the seat reservation requests
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

//FIFO that receives the answer to the client

int create_fifo_resposta_read()
{
	int fd;
	char *resposta = "/tmp/resposta";
	char buf[MAX_BUF];

	// open, read, and display the message from the FIFO
	fd = open(resposta, O_RDONLY);
	read(fd, buf, MAX_BUF);
	printf("Received: %s\n", buf);
	close(fd); //suposto fechar aqui????

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		printf("Use: %s <time_out> <num_wanted_seats> <pref_seat_list>\n", *argv);
		return 0;
	}

	unsigned int time_out, num_wanted_seats;
	unsigned int seat_counter=argc-3;
	int pref_seat_list[seat_counter];

	if (sscanf(argv[1], "%u", &time_out) < 1 || sscanf(argv[2], "%u", &num_wanted_seats) < 1)
	{
		printf("Invalid input\n");
		return -1;
	}

	if(seat_counter<num_wanted_seats){
		printf("Not enough seats specified\n");
		return -1;
	 }

	if (time_out > MAX_TIME_OUT)
		printf("Timeout too big\n");


	if (num_wanted_seats > MAX_SEATS || num_wanted_seats < 1){
		printf("Too much wanted seats\n");
		return -1;
	}

	char* p;
	errno = 0;
	for(unsigned int i=0;i<seat_counter;i++){
		pref_seat_list[i] = strtol(argv[i+3], &p, 10);
		//testa se é integer
		if (*p != '\0' || errno != 0){
			printf("Preaferable seats must be integers\n");
			return -1;
		}
	}

	return 0;
}
