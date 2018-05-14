#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include "queue.h"
#include "ticket_office.h"

#define MAX_ROOM_SEATS				9999
#define MAX_CLI_SEATS				99
#define WIDTH_PID					5
#define WIDTH_XXNN					5
#define WIDTH_SEAT					4
#define MAX_TICKET_OFFICES_NUMBER	100

#define BUFFER_SIZE					4096
#define REQUESTS_BUFFER_SIZE		1

// parâmetros necessários para a execução de cada thread
struct ticket_office_thr_params {
	int ticket_office_id;
	int num_seats;
	FILE *slog;
	Seat *seats;
	queue *requests_buffer;
};

// declaração da função a executar pelos threads
void *ticket_office_func(void *params);
void ticket_office_cleanup(void *params);

// entry point
int main(int argc, const char* argv[]) {

	// verificação do correcto número de parâmetros
	if (argc < 3) {
		printf("Use: %s <num_seats> <num_ticket_offices> <open_time>\n", *argv);
		exit(EXIT_SUCCESS);
	}

	// lê inteiros a partir dos parâmetros da função main. Se algum for inválido aborta a execução
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

	// cria o FIFO para requests, a ser usado pelos clientes para enviar requests
	const char *requests_fifo = "/tmp/requests";
	if (mkfifo(requests_fifo, 0666) < 0) {
		// erro na criação do FIFO, se o erro for já existir e não poder ser então criado, usa-se o FIFO já existente.
		// Se for outro erro, aborta a execução
		if (errno != EEXIST) {
			perror("mkfifo requests");
			exit(EXIT_SUCCESS);
		}
		else
			printf("%s already exists, using previously created FIFO\n", requests_fifo);
	} printf("requests_fifo created\n");

	// abre o FIFO requests em modo de leitura.
	// O_NONBLOCK é especificado para a chamada ao sistema não bloquear à espera que o outro extremo seja também aberto
	int fifo_fd = open(requests_fifo, O_RDONLY|O_NONBLOCK);
	if (fifo_fd < 0) {
		perror("open requests fifo");
		goto unlink_requests_fifo;
	} printf("requests_fifo opened\n");

	// aloca o array de seats que representa a sala com o número num_seats fornecido
	Seat *seats = malloc(num_seats * sizeof(*seats));
	if (!seats) {
		perror("malloc seat array");
		goto close_fifo_fd;
	} printf("Seat array created\n");
	pthread_mutexattr_t mut_attr;
	pthread_mutexattr_init(&mut_attr);
	pthread_mutexattr_settype(&mut_attr, PTHREAD_MUTEX_ERRORCHECK);
	for (int i = 0; i < num_seats; ++i) {
		seats[i].free = true;
		seats[i].clientId = 0;
		pthread_mutex_init(&seats[i].mut, &mut_attr);
	}

	// aloca o buffer para requests; o seu tamanho deve ser unitário como especificado no enunciado.
	queue *requests_buffer;
	if (queue_new(&requests_buffer, REQUESTS_BUFFER_SIZE) < 0) {
		fprintf(stderr, "Error creating queue for requests\n");
		goto free_seat_array;
	} printf("Requests buffer created\n");

	// abertura do ficheiro "slog.txt" para escrita. Poderia ser aberto por cada thread individualmente,
	// mas como ficheiros abertos são partilhados entre threads, evita todos os problemas de concurrência
	// na sua escrita por cada thread, desde que a escrita seja line-buffered, que é o caso.
	const char *slog_filename = "slog.txt";
	FILE *slog = fopen(slog_filename, "w");
	if (!slog) {
		perror("fopen slog");
		goto free_requests_buffer;
	} printf("slog file created\n");

	// criação de um array de identificadores de thread com o tamanho de bilheteiras existentes
	pthread_t *ticket_office_thr = malloc(num_ticket_offices * sizeof(*ticket_office_thr));
	if (!ticket_office_thr) {
		perror("malloc ticket_office_thr");
		goto fclose_slog;
	} printf("ticket_office_thr created\n");

	// struct ticket_office_thr_params (totp) a ser passada a cada thread; memória é libertada pelos threads
	struct ticket_office_thr_params *totp;
	for (int i = 0; i < num_ticket_offices; ++i) {
		totp = malloc(sizeof(*totp));
		if (!totp) {
			perror("malloc struct totp");
			goto free_ticket_office_thr;
		}
		totp->slog = slog;
		totp->seats = seats;
		totp->requests_buffer = requests_buffer;
		totp->num_seats = num_seats;
		totp->ticket_office_id = i;
		if ((errno = pthread_create(ticket_office_thr + i, NULL, ticket_office_func, totp) < 0)) {
			perror("pthread_create");
			for (int j = 0; j < i; ++j) {
				pthread_cancel(ticket_office_thr[j]);
				pthread_join(ticket_office_thr[j], NULL);
			}
			free(totp);
			goto free_ticket_office_thr;
		}
	} printf("threads started\n");

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fifo_fd, &fds);
	struct timeval tv = { .tv_sec = open_time, .tv_usec = 0 };
	while (tv.tv_sec) {
		int ret = select(fifo_fd + 1, &fds, NULL, NULL, &tv);
		if (ret < 1) {
			if (ret < 0)
				perror("select");
			else
				printf("timeout\n");
			break;
		}
		if (FD_ISSET(fifo_fd, &fds)) {
			char *buffer = malloc(BUFFER_SIZE * sizeof(*buffer));
			if (!buffer) {
				perror("malloc buffer");
				break;
			}
			int n = read(fifo_fd, buffer, BUFFER_SIZE - 1);
			if (n < 0) {
				free(buffer);
				perror("read fifo");
				break;
			}
			if (!n) {
				fprintf(stderr, "write end of requests fifo closed\n");
				free(buffer);
				break;
			}
			buffer[n] = 0;
			// memory shared between threads must be dinamically allocated...
			printf("queue_put returned %d. put %d-%.*s\n", queue_put(requests_buffer, buffer), n, n, buffer);
		}
		else {
			fprintf(stderr, "Unknown error in select\n");
			break;
		}
	}

	// O timeout deve ser implementado neste thread (main thread),
	// que sinalizará todos os outros aquando do seu término por forma a estes terminarem a sua execução: por implementar

	// sinaliza todos os threads que estes devem terminar as operações correntes, libertar os seus recursos e terminar a execução
	for (int i = 0; i < num_ticket_offices; ++i)
		pthread_cancel(ticket_office_thr[i]);

	// esperar pelo término da execução de cada thread
	for (int i = 0; i < num_ticket_offices; ++i) {
		if (pthread_join(ticket_office_thr[i], NULL) < 0)
			perror("pthread_join");
	} printf("threads joined\n");

	// abertura do ficheiro "sbook.txt" para escrita dos lugares reservados pelas bilheteiras
	const char *sbook_filename = "sbook.txt";
	FILE *sbook = fopen(sbook_filename, "w");
	if (!sbook) {
		perror("fopen sbook");
		goto free_ticket_office_thr;
	} printf("sbook created\n");

	// escrita no ficheiro sbook dos lugares efectivamente reservados.
	// Não é necessário bloquear mutexes dos lugares porque só há um thread a ler estes valores.
	for (int i = 0; i < num_seats; ++i)
		if (!seats[i].free)
			fprintf(sbook, "%0*d\n", WIDTH_SEAT, i);
	goto fclose_sbook;

fclose_sbook:
	if (fclose(sbook) < 0)
		perror("fclose sbook");
	printf("sbook closed\n");

free_ticket_office_thr:
	free(ticket_office_thr);
	printf("ticket_office_thr freed\n");

fclose_slog:
	if (fclose(slog) < 0)
		perror("fclose slog");
	printf("slog closed\n");

free_requests_buffer:
	queue_free(requests_buffer);
	printf("requests_buffer freed\n");

close_fifo_fd:
	if (close(fifo_fd) < 0)
		perror("close requests fifo");
	printf("requests_fifo closed\n");

free_seat_array:
	free(seats);
	printf("seat array freed\n");

unlink_requests_fifo:
	if (unlink(requests_fifo) < 0)
		perror("unlink requests fifo");
	printf("requests_fifo unlinked\n");

	return EXIT_SUCCESS;
}

void *ticket_office_func(void *params) {
	struct ticket_office_thr_params *args = params;
	FILE *slog = args->slog;
	int thr_id_width = snprintf(NULL, 0, "%d", MAX_TICKET_OFFICES_NUMBER - 1);
	printf("to%0*d started...\n", thr_id_width, args->ticket_office_id);
	fprintf(slog, "%02d-OPEN\n", args->ticket_office_id);
	pthread_cleanup_push(ticket_office_cleanup, args);
	char *request;

	for (;;) {
		if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL))
			fprintf(stderr, "to%0*d: Unable to set cancel state. Thread may not be "
			                "canceled and run in an infinite loop.\n", thr_id_width, args->ticket_office_id);
		pthread_testcancel();

		if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL))
			fprintf(stderr, "to%0*d: Unable to set cancel type to aynchrnous. Thread may not be "
			                "canceled and run in an infinite loop.\n", thr_id_width, args->ticket_office_id);

		printf("to%0*d waiting for msg in queue...\n", thr_id_width, args->ticket_office_id);
		// get request from requests_buffer, this operation blocks until there is something on it
		if (queue_take(args->requests_buffer, &request) < 0) {
			fprintf(stderr, "to%0*d: Error taking request from queue\n", thr_id_width, args->ticket_office_id);
			pthread_exit(NULL);
		}
		printf("to%0*d received message from queue: %s\n", thr_id_width, args->ticket_office_id, request);

		if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL))
			fprintf(stderr, "to%0*d: Unable to set cancel state. Thread may be canceled "
			                "sooner than expected, unable to free some resources.\n", thr_id_width, args->ticket_office_id);

		int client_pid, num_wanted_seats, *pref_seat_list = NULL;
		int offset = 0, n;
		if (sscanf(request, "%d %n", &client_pid, &n) < 1) {
			fprintf(stderr, "to%0*d: Error reading client pid from request\n", thr_id_width, args->ticket_office_id);
			free(request);
			pthread_exit(NULL);
		}// printf("to%0*d client_pid: %d\n", thr_id_width, args->ticket_office_id, client_pid);
		offset += n;

		char fifo_name[9 + WIDTH_PID] = "/tmp/ans";
		sprintf(fifo_name + 8, "%0*d", WIDTH_PID, client_pid);
		FILE *fifo = fopen(fifo_name, "w");
		if (!fifo) {
			fprintf(stderr, "to%0*d: fifo open:", thr_id_width, args->ticket_office_id);
			perror(NULL);
			free(request);
			pthread_exit(NULL);
		}// printf("to%0*d opened fifo %s\n", thr_id_width, args->ticket_office_id, fifo_name);

		int exit_code = 0;
		if (sscanf(request + offset, "%d %n", &num_wanted_seats, &n) < 1) {
			fprintf(stderr, "to%0*d: Error reading client pid from request\n", thr_id_width, args->ticket_office_id);
			goto free_request;
		}// printf("to%0*d num_wanted_seats: %d\n", thr_id_width, args->ticket_office_id, num_wanted_seats);
		offset += n;

		if (num_wanted_seats < 1) {
			fprintf(stderr, "to%0*d: Number of wanted seats is too small.\n", thr_id_width, args->ticket_office_id);
			exit_code = -4;
			goto fifo_print_exit_code;
		}
		if (num_wanted_seats > MAX_CLI_SEATS) {
			fprintf(stderr, "to%0*d: Number of wanted seats is too big.\n", thr_id_width, args->ticket_office_id);
			exit_code = -1;
			goto fifo_print_exit_code;
		}

		int num_pref_seat = 0;
		for (int c, c_size = 0, size = strlen(request + offset)+offset; offset < size; offset += n) {
			if (sscanf(request + offset, "%d %n", &c, &n) < 1) {
				int *tmp = realloc(pref_seat_list, num_pref_seat * sizeof(*pref_seat_list));
				if (!tmp) {
					perror("realloc");
					break;
				}
				pref_seat_list = tmp;
				break;
			}
			if (c < 0 || c > args->num_seats) {
				exit_code = -3;
				goto free_pref_seat_list;
			}
			if (num_pref_seat == c_size) {
				c_size = c_size? c_size * 2: 1;
				int *tmp = realloc(pref_seat_list, c_size * sizeof(*pref_seat_list));
				if (!tmp) {
					perror("realloc");
					break;
				}
				pref_seat_list = tmp;
			}
			pref_seat_list[num_pref_seat++] = c;
		}// printf("to%0*d read %d seat preferences\n", thr_id_width, args->ticket_office_id, num_pref_seat);
		if (num_pref_seat < num_wanted_seats || num_pref_seat > MAX_CLI_SEATS) {
			fprintf(stderr, "to%0*d: Invalid number of seat preferences.\n", thr_id_width, args->ticket_office_id);
			exit_code = -2;
			goto free_pref_seat_list;
		}
		printf("to%0*d seat preferences: ", thr_id_width, args->ticket_office_id);
		for (int i = 0; i < num_pref_seat; ++i)
			printf("%d ", pref_seat_list[i]);
		printf("\n");

		int *allocated_seats = malloc(num_wanted_seats * sizeof(*allocated_seats));
		if (!allocated_seats) {
			perror("malloc allocated_seats");
			goto free_pref_seat_list;
		} //printf("to%0*d allocated_seats memory allocated\n", thr_id_width, args->ticket_office_id);
		
		int num_allocated_seats = 0;
		for (int i = 0; i < num_pref_seat && num_allocated_seats < num_wanted_seats; ++i) {
			if (isSeatFree(args->seats, pref_seat_list[i])) {
				printf("to%0*d wanted seat %d is free, booking it\n", thr_id_width, args->ticket_office_id, pref_seat_list[i]);
				bookSeat(args->seats, pref_seat_list[i], client_pid);
				allocated_seats[num_allocated_seats++] = pref_seat_list[i];
			}
			else
				printf("to%0*d wanted seat %d is already taken\n", thr_id_width, args->ticket_office_id, pref_seat_list[i]);
		} printf("to%0*d booked %d seats\n", thr_id_width, args->ticket_office_id, num_allocated_seats);


		if (num_allocated_seats < num_wanted_seats) {
			for (int i = 0; i < num_allocated_seats; ++i)
				freeSeat(args->seats, allocated_seats[i]);
			printf("to%0*d insufficient number of seats, freeing them...\n", thr_id_width, args->ticket_office_id);
			exit_code = -5;
		}

		int num_cli_seats_width = snprintf(NULL, 0, "%d", MAX_CLI_SEATS - 1);
		int seat_id_width = snprintf(NULL, 0, "%d", args->num_seats - 1);
		fprintf(slog, "%0*d-%0*d-%0*d: ", thr_id_width, args->ticket_office_id, WIDTH_PID, client_pid, num_cli_seats_width, num_wanted_seats);
		for (int i = 0; i < num_wanted_seats; ++i)
			fprintf(slog, "%0*d ", seat_id_width, pref_seat_list[i]);
		fprintf(slog, "- ");
		const char *error_codes[] = { "", "MAX", "NST", "IID", "ERR", "NAV", "FUL" };
		if (exit_code)
			fprintf(slog, "%s", error_codes[-exit_code]);
		else
			for (int i = 0; i < num_allocated_seats; ++i)
				fprintf(slog, "%0*d ", seat_id_width, allocated_seats[i]);
		fprintf(slog, "\n");

		if (!exit_code) {
			// use cleaner write instead of fprintf to ensure atomic writes to file
			fprintf(fifo, "%d ", num_allocated_seats);
			for (int i = 0; i < num_allocated_seats; ++i)
				fprintf(fifo, "%d ", allocated_seats[i]);
			fprintf(fifo, "\n");
		}
		else
			fprintf(fifo, "%d\n", exit_code);


		free(allocated_seats);

	free_pref_seat_list:
		free(pref_seat_list);

	fifo_print_exit_code:
		if (exit_code)
			fprintf(fifo, "%d\n", exit_code);

	free_request:
		free(request);

		if (fclose(fifo) < 0)
			perror("fclose fifo");

	}
	pthread_cleanup_pop(1);
	return NULL;
}

void ticket_office_cleanup(void *params) {
	struct ticket_office_thr_params *args = params;
	fprintf(args->slog, "%02d-CLOSED\n", args->ticket_office_id);
	pthread_mutex_unlock(&args->requests_buffer->mut);
	free(args);
	return;
}
