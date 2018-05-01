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
//Escrita em slog.txt
//escrita em sbook.txt
//Criar funcao que controla o tempo que o server esta "online"
//return de execcao ao cliente se parametros de pedido forem invalidos
//estrutura de semaforos

//Defines

#define MAX_ROOM_SEATS = 140
#define MAX_CLI_SEATS = 100
#define WIDTH_SEAT = 4
#define MAX_BUF 1024

//Function that initializes the server
int startserver(char *num_room_seats, char *num_ticket_offices, char *open_time)
{

}

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

int main(int agrc, char *argv[])
{

	startserver(argv[1], argv[2], argv[3]);

	return 0;
}