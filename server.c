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

//TO-DOS

//CRIAR MAIN THREAD
//CRIAR THREADS num_ticket_offices (geradas por thread main)
//int isSeatFree(Seat *seats, int seatNum) – testa se o lugar seatNum esta livre
/*void bookSeat(Seat *seats, int seatNum, int clientId) – reserva o lugar seatNum
para o cliente cujo identificador e clientId */
/*void freeSeat(Seat *seats, int seatNum) – liberta o lugar seatNum, 
apoos uma tentativa de reserva sem sucesso, 
em que foram pre-reservados alguns lugares mas nao 
foi possivel reservar todos os lugares pretendidos.*/
//Escrita em slog.txt (tem que ser aberto na main thread, usar fprintf e fread)
//escrita em sbook.txt (tem que ser aberto na main thread, usar fprintf e fread)
//Criar funcao que controla o tempo que o server esta "online"
//return de execcao ao cliente se parametros de pedido forem invalidos
//estrutura de semaforos
/*criar um array que representa a sala para marcar os lugares ocupados 
para não haver necessidade de ir ao ficheiro txt*/

//Defines

#define MAX_ROOM_SEATS 140
#define MAX_CLI_SEATS 100
#define WIDTH_SEAT 4
#define MAX_BUF 1024
#define MAX_TICKET_OFFICES 5
#define MAX_OPEN_TIME 1000

//FIFO que recebe os pedidos de reserva de lugar
int create_fifo_request_read()
{
	int fd;
	char *request = "/tmp/request";
	char buf[MAX_BUF];

	// open, read, and display the message from the FIFO
	fd = open(request, O_RDONLY);
	read(fd, buf, MAX_BUF);
	printf("Received: %s\n", buf);
	close(fd);

	return 0;
}

int create_fifo_resposta_write()
{
	//FIFO deve ter o nome ansXXXXX, em que XXXXX representa o PID do cliente (TO-DO)
	int fd;
	char *resposta = "/tmp/resposta";

	// create the FIFO (named pipe)
	mkfifo(resposta, 0666);

	// Write something to fifo
	fd = open(resposta, O_WRONLY);
	write(fd, "Hi", sizeof("Hi"));
	close(fd);

	// remove the FIFO
	unlink(resposta);

	return 0;
}

int main(int argc, const char *argv[])
{
	//to-do verificar args
	if (argc < 4)
	{
		printf("Use: %s <num_room_seats> <num_ticket_office> <open_time>\n", *argv);
		return 0;
	}

	unsigned int num_room_seats, num_ticket_office, open_time;

	if (sscanf(argv[1], "%u", &num_room_seats) < 1 || sscanf(argv[2], "%u", &num_ticket_office) < 1 || sscanf(argv[3], "%u", &open_time) < 1)
	{
		printf("Invalid input\n");
		return -1;
	}
	if (num_room_seats > MAX_ROOM_SEATS)
		printf("More seats than expected\n");

	if (num_ticket_office > MAX_TICKET_OFFICES)
		printf("More ticket offices than expected\n");

	if (open_time > MAX_OPEN_TIME)
		printf("Open time over the expected");

	//inicializar sala
	bool lugar[num_room_seats];
	pthread_mutex_t mut[num_room_seats];

	return 0;
}