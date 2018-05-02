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
#define MAX_TIME_OUT 100
#define MAX_SEATS 99
#define MAX_BUF 1024
//TO-DOS

//escrita em clog.txt
//escrita em cbook.txt
//criar metodo de leitura do argumento <pref_seat_list>

//Function that initializes the client
int client_init(char *time_out, char *num_wanted_seats, char *pref_seat_list)
{
	return 0;
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

//Space count of the argv[4]
int chrcount(const char *str, char chr)
{
	if (!str)
		return -1;
	int cnt = 0;
	for (const char *p = str; *p; ++p)
		if (*p == chr)
			++cnt;
	return cnt;
}

int main(int argc, char *argv[])
{
	unsigned int time_out, num_wanted_seats,*pref_seat_list;

	if (argc < 4)
	{
		printf("Use: %s <time_out> <num_wanted_seats> <pref_seat_list>\n", *argv);
		return 0;
	}

	if (sscanf(argv[1], "%u", &time_out) < 1 || sscanf(argv[2], "%u", &num_wanted_seats) < 1)
	{
		printf("Invalid input\n");
		return -1;
	}

	/*if(seat_counter<num_wanted_seats){
		printf("Not enough seats specified\n");
		return -1;
	 }*/

	if (time_out > MAX_TIME_OUT)
	{
		printf("Timeout too big\n");
		return -1;
	}

	if (num_wanted_seats > MAX_SEATS || num_wanted_seats < 1)
	{
		printf("Too much wanted seats\n");
		return -1;
	}
	int aux=chrcount(argv[3],' ')+1;
	pref_seat_list=malloc(aux*sizeof(unsigned));
	
	if(pref_seat_list==NULL){
	perror("Malloc");
	return -1;
	}
	int n_pref=0;
	for(int i=0,cnt_aux=0;i<=aux;i++){
		int n;
		if(sscanf(argv[3]+cnt_aux,"%u%n",&pref_seat_list[i],&n)<1)
		break;
		cnt_aux+=n;
		n_pref++;
	}

	if(n_pref<aux){
		printf("<pref_seat_list> is WRONG! Nice try :)\n");
		return -1;
	}

	for(int i=0;i<n_pref;i++)
	printf("%u\n",pref_seat_list[i]);

	return 0;
}