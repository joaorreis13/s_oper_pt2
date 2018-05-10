#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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

//estrutura de semaforos
/*criar um array que representa a sala para marcar os lugares ocupados 
para não haver necessidade de ir ao ficheiro txt*/



#define MAX_ROOM_SEATS			9999
#define MAX_CLI_SEATS			99
#define WIDTH_PID				5
#define WIDTH_XXNN				5
#define WIDTH_SEAT				4

#define BUFFER_SIZE				4096
#define REQUESTS_BUFFER_SIZE	1

struct ticket_office_thr_params {
	int ticket_office_id;
	FILE *slog;
	intptr_t seats;
	char **request_buffer;
	pthread_cond_t *request_buffer_cond;
};

typedef struct{
	pid_t pid_client;
	int booked; //boolean
} Seat;

void *ticket_office_func(void *params);

int main(int argc, const char* argv[]) {

	if (argc < 3) {
		printf("Use: %s <num_seats> <num_ticket_offices> <open_time>\n", *argv);
		exit(EXIT_SUCCESS);
	}

	int num_seats, num_ticket_offices, open_time;
	if (sscanf(argv[1], "%d", &num_seats) < 1 || num_seats < 0 || num_seats > MAX_ROOM_SEATS) {
		printf("Invalid number of seats\n");
		exit(EXIT_SUCCESS);
	}
	if (sscanf(argv[2], "%d", &num_ticket_offices) < 1 || num_ticket_offices < 0) {
		printf("Invalid number of ticket offices\n");
		exit(EXIT_SUCCESS);
	}
	if (sscanf(argv[3], "%d", &open_time) < 1 || open_time < 0) {
		printf("Invalid open time\n");
		exit(EXIT_SUCCESS);
	}
	printf("Number of seats: %d\nNumber of ticket offices: %d\nOpen time: %d(seconds)\n",
	        num_seats, num_ticket_offices, open_time);

	const char *requests_fifo = "/tmp/requests";
	if (mkfifo(requests_fifo, 0666) < 0) {
		if (errno != EEXIST) {
			perror("mkfifo requests");
			exit(EXIT_SUCCESS);
		}
		else
			printf("%s already exists, using previously created FIFO\n", requests_fifo);
	} printf("requests_fifo created\n");

	int fifo_fd = open(requests_fifo, O_RDONLY|O_NONBLOCK);
	if (fifo_fd < 0) {
		perror("open requests fifo");
		goto unlink_requests_fifo;
	} printf("requests_fifo opened\n");

	char **requests_buffer = malloc(REQUESTS_BUFFER_SIZE * sizeof(*requests_buffer));
	if (!requests_buffer) {
		perror("malloc atomic requests_buffer");
		goto close_fifo_fd;
	}
	*requests_buffer = NULL;
	printf("requests_buffer created\n");

	const char *slog_filename = "slog.txt";
	FILE *slog = fopen(slog_filename, "w");
	if (!slog) {
		perror("fopen slog");
		goto free_requests_buffer;
	} printf("slog file created\n");

	pthread_t *ticket_office_thr = malloc(num_ticket_offices * sizeof(*ticket_office_thr));
	if (!ticket_office_thr) {
		perror("malloc ticket_office_thr");
		goto fclose_slog;
	} printf("ticket_office_thr created\n");

	pthread_cond_t request_buffer_cond = PTHREAD_COND_INITIALIZER;

	struct ticket_office_thr_params p = {
		.slog = slog,
		.seats = 0,
		.request_buffer = requests_buffer,
		.request_buffer_cond = &request_buffer_cond,
	};
	for (int i = 0; i < num_ticket_offices; ++i) {
		p.ticket_office_id = i;
		if (errno = pthread_create(ticket_office_thr + i, NULL, ticket_office_func, NULL) < 0) {
			perror("pthread_create");
			goto free_ticket_office_thr;
			//	missing waiting for already created threads in case one fails
		}
	} printf("threads started\n");

	// wait for open time to expire

	for (int i = 0; i < num_ticket_offices; ++i) {
		if (pthread_join(ticket_office_thr[i], NULL) < 0) {
			perror("pthread_join");
		}
	} printf("threads joined\n");

	const char *sbook_filename = "sbook.txt";
	FILE *sbook = fopen(sbook_filename, "w");
	if (!sbook) {
		perror("fopen sbook");
		goto free_ticket_office_thr;
	} printf("sbook created\n");




fclose_sbook:
	if (fclose(sbook) < 0) {
		perror("fclose sbook");
	} printf("sbook closed\n");

free_ticket_office_thr:
	free(ticket_office_thr);
	printf("ticket_office_thr freed\n");

fclose_slog:
	if (fclose(slog) < 0) {
		perror("fclose slog");
	} printf("slog closed\n");

free_requests_buffer:
	free(requests_buffer);
	printf("requests_buffer freed\n");

close_fifo_fd:
	if (close(fifo_fd) < 0) {
		perror("close requests fifo");
	} printf("requests_fifo closed\n");

unlink_requests_fifo:
	if (unlink(requests_fifo) < 0) {
		perror("unlink requests fifo");
	} printf("requests_fifo unlinked\n");

	return EXIT_SUCCESS;
}

void *ticket_office_func(void *params) {
	
	return NULL;
}
