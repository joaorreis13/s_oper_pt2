#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_ROOM_SEATS			9999
#define MAX_CLI_SEATS			99
#define WIDTH_PID				5
#define WIDTH_XXNN				5
#define WIDTH_SEAT				4

#define BUFFER_SIZE				4096
#define REQUESTS_BUFFER_SIZE	1

// parâmetros necessários para a execução de cada thread
struct ticket_office_thr_params {
	int ticket_office_id;
	FILE *slog;
	intptr_t seats;
	char **request_buffer;
	pthread_cond_t *request_buffer_cond;
};

// não sei se isto veio de algum ficheiro do prof ou se é suposto fazer alguma coisa com isto
typedef struct{
	pid_t pid_client;
	int booked;
} Seat;

// declaração da função a executar pelos threads
void *ticket_office_func(void *params);

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

	// alloca o buffer para requests; o seu tamanho deve ser unitário como especificado no enunciado e portanto
	// a alocação não necessita de ser manual. No entanto, desta forma é mais fácil posteriormente mudar o
	// tamanho do buffer através da macro REQUESTS_BUFFER_SIZE
	char **requests_buffer = malloc(REQUESTS_BUFFER_SIZE * sizeof(*requests_buffer));
	if (!requests_buffer) {
		perror("malloc requests_buffer");
		goto close_fifo_fd;
	}
	*requests_buffer = NULL;
	printf("requests_buffer created\n");

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

	// pthread_cond_t a usar pelos threads para sinalizar que o requests_buffer tem algo para ser retirado
	pthread_cond_t request_buffer_cond = PTHREAD_COND_INITIALIZER;

	// struct ticket_office_thr_params (totp) a ser passada a cada thread
	struct ticket_office_thr_params *totp;
	// criação dos threads. Falta: se houver um erro na alocação de memória dos parâmetros a ser passados ou na
	// própria criação dos threads, é necessário matar e esperar pelos threads já criados
	for (int i = 0; i < num_ticket_offices; ++i) {
		totp = malloc(sizeof(*totp));
		if (!totp) {
			perror("malloc struct totp");
			goto free_ticket_office_thr;
		}
		totp->slog = slog;
		totp->seats = 0;
		totp->request_buffer = requests_buffer;
		totp->request_buffer_cond = &request_buffer_cond;
		totp->ticket_office_id = i;
		if (errno = pthread_create(ticket_office_thr + i, NULL, ticket_office_func, totp) < 0) {
			perror("pthread_create");
			goto free_ticket_office_thr;
			//	missing waiting for already created threads in case one fails
		}
	} printf("threads started\n");

	// wait for opening time to expire
	// O timeout deve ser implementado neste thread (main thread),
	// que sinalizará todos os outros aquando do seu término por forma a estes terminarem a sua execução: por implementar

	// esperar pelo término da execução de cada thread
	for (int i = 0; i < num_ticket_offices; ++i) {
		if (pthread_join(ticket_office_thr[i], NULL) < 0) {
			perror("pthread_join");
		}
	} printf("threads joined\n");

	// abertura do ficheiro "sbook.txt" para escrita dos lugares reservados pelas bilheteiras
	const char *sbook_filename = "sbook.txt";
	FILE *sbook = fopen(sbook_filename, "w");
	if (!sbook) {
		perror("fopen sbook");
		goto free_ticket_office_thr;
	} printf("sbook created\n");

	// falta fazer a escrita no ficheiro sbook do resultado apropriado


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
	free(requests_buffer);
	printf("requests_buffer freed\n");

close_fifo_fd:
	if (close(fifo_fd) < 0)
		perror("close requests fifo");
	printf("requests_fifo closed\n");

unlink_requests_fifo:
	if (unlink(requests_fifo) < 0)
		perror("unlink requests fifo");
	printf("requests_fifo unlinked\n");

	return EXIT_SUCCESS;
}

// função a ser executada por cada thread; por implementar
/* 
 * Em falta:
 * 	Registar a abertura da bilheteira no ficheiro slog;
 * 	Esperar, através da pthread_cond_t que o buffer tenha um request;
 * 	Retirar o pedido do buffer e processá-lo;
 * 	Se tiver valores inválidos retornar resposta apropriada para o FIFO respectivo;
 * 	Começar a reservar os lugares sequencialmente a partir da lista de preferências através da API especificada no enunciado;
 * 	Invocar a macro DELAY() em cada chamada a esta API;
 * 	Após o pedido satisfeito ou impossibilidade de atender o pedido fechar a bilheteira;
 * 		Razões para a impossibilidade do atendimento do pedido:
 * 			Fim da lista de preferências atingido;
 * 			Timeout sinalizado pelo main thread;
 * 			Sala cheia;
 * 	Registar o fecho da bilheteira no ficheiro slog;
 */
void *ticket_office_func(void *params) {
	
	return NULL;
}
