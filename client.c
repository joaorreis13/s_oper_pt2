#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define WIDTH_PID		5
#define WIDTH_SEAT		4
#define BUFFER_SIZE		4096

int main(int argc, const char *argv[]) {
	//	código do prof
	printf("** Running process %d (PGID %d) **\n", getpid(), getpgrp());

	if (argc == 4)
		printf("ARGS: %s | %s | %s\n", argv[1], argv[2], argv[3]);

	sleep(1);
	// código do prof termina aqui

	// verificar se o número de argumentos está correcto
	if (argc < 4) {
		printf("Usage: %s <timeout> <num_wanted_seats> <pref_seat_list>\n", *argv);
		exit(EXIT_FAILURE);
	}

	int timeout, num_wanted_seats, *pref_seat_list = NULL;
	// converter o primeiro argumento num inteiro;
	if (sscanf(argv[1], "%d", &timeout) < 1 || timeout < 0) {
		printf("Invalid timeout\n");
		exit(EXIT_FAILURE);
	}
	if (sscanf(argv[2], "%d", &num_wanted_seats) < 1 || num_wanted_seats < 0) {
		printf("Invalid number of wanted seats\n");
		exit(EXIT_FAILURE);
	}
	printf("Timeout is %u\nNumber of wanted seats is %u\n", timeout, num_wanted_seats);

	// percorrer o 3º argumento a procurar inteiros que representam o número de um lugar correspondente à
	// preferência. Os números encontrados são guardados no array dinamicamente alocado pref_seat_list.
	// para evitar usar o realloc a cada iteração, a memória alocada cresce sempre para o dobro quando fica cheia
	// e o tamanho final é reajustado quando o ciclo acaba.
	int num_pref_seat = 0;
	for (int c = 0, offset = 0, n, c_size = 0, size = strlen(argv[3]); offset < size; offset += c) {
		if (sscanf(argv[3] + offset, "%d%n", &n, &c) < 1) {
			// formato de número inválido, ajusta o tamanho do array para apenas o necessário e sai do ciclo
			int *tmp = realloc(pref_seat_list, num_pref_seat * sizeof(*pref_seat_list));
			if (!tmp) {
				perror("realloc");
				break;
			}
			pref_seat_list = tmp;
			break;
		}
		if (n < 0)
			continue;
		if (num_pref_seat == c_size) {
			// se a memória actualmente alocada já estiver cheia, duplicar o tamanho do array
			c_size = c_size? c_size * 2: 1;
			int *tmp = realloc(pref_seat_list, c_size * sizeof(*pref_seat_list));
			if (!tmp) {
				perror("realloc");
				break;
			}
			pref_seat_list = tmp;
		}
		// guardar o número encontrado no array
		pref_seat_list[num_pref_seat++] = n;
	}

	// número de preferências encontradas é menor que o número de lugares pretendidos
	if (num_pref_seat < num_wanted_seats) {
		printf("Insuficient number of seat preferences\n");
		goto free_pref_seat_list;
	}

	printf("seat preferences: ");
	// imprime os números das preferências encontrados
	for (int i = 0; i < num_pref_seat; ++i)
		printf("%d ", pref_seat_list[i]);
	printf("\n");

	// cria um FIFO com o nome "/tmp/ansxxxxx", correspondente ao PID
	char fifo_name[WIDTH_PID + 8] = "/tmp/ans";
	sprintf(fifo_name + 8, "%0*d", WIDTH_PID, getpid());
	if (mkfifo(fifo_name, 0666) < 0) {
		perror("mkfifo");
		goto free_pref_seat_list;
	} printf("created fifo %s\n", fifo_name);

	// abre o FIFO criado só para leitura, por onde vai receber a resposta do servidor
	// O_NONBLOCK é especificado para a chamada ao sistema não bloquear enquanto o outro lado do FIFO não for aberto
	int fifo = open(fifo_name, O_RDONLY|O_NONBLOCK);
	if (fifo < 0) {
		perror("fifo open");
		goto unlink_fifo;
	} printf("fifo opened\n");

	// imprime num buffer a mensagem que vai enviar ao servidor através do FIFO requests no formato especificado pelo prof
	char buffer[BUFFER_SIZE];
	int n = 0;
	snprintf(buffer, BUFFER_SIZE, "%d %d %n", getpid(), num_wanted_seats, &n);
	for (int i = 0, c; i < num_pref_seat; ++i, n += c)
		snprintf(buffer + n, BUFFER_SIZE - n, "%d %n", pref_seat_list[i], &c);
	buffer[n - 1] = '\n';
	printf("buffer: %s\n", buffer);

	// abre o FIFO "/tmp/requests" para escrita
	int requests_fifo = open("/tmp/requests", O_WRONLY);
	if (requests_fifo < 0) {
		perror("requests fifo open");
		goto close_fifo;
	} printf("requests fifo opened\n");

	// escreve o conteúdo do buffer no FIFO
	if (write(requests_fifo, buffer, n) < 0) {
		perror("requests fifo write");
		goto close_requests_fifo;
	} printf("wrote '%*s' to requests fifo\n", n, buffer);

	// ler do FIFO de resposta com timeout
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fifo, &fds);
	struct timeval tv = { .tv_sec = timeout, .tv_usec = 0 };
	int ret = select(fifo + 1, &fds, NULL, NULL, &tv);
	if (ret < 1) {
		if (ret < 0) {
			perror("select");
			goto close_requests_fifo;
		}
		else {
			printf("Server response timed out. Exiting.\n");
			ret = -7; // OUT in error_codes
			goto print_clog;
		}
	}
	if (FD_ISSET(fifo, &fds)) {
		char buffer[BUFFER_SIZE];
		int n = read(fifo, buffer, BUFFER_SIZE - 1);
		if (n < 0) {
			perror("read fifo");
			goto close_requests_fifo;
		}
		if (!n) {
			fprintf(stderr, "write end of fifo closed\n");
			goto close_requests_fifo;
		}
		buffer[n] = 0;
	}
	else {
		fprintf(stderr, "Unknown error in select\n");
		goto close_requests_fifo;
	}

	printf("received response: %d'%.*s'\n", n, n, buffer);

	int offset;
	sscanf(buffer, "%d %n", &ret, &offset);
	const char *error_codes[] = { "", "MAX", "NST", "IID", "ERR", "NAV", "FUL", "OUT" };
	if (ret < 0) {
		printf("Server unable to book wanted seats. Error code: %s\n", error_codes[-ret]);
		goto print_clog;
	}
	if (ret != getpid()) {
		printf("Unexpected parameter: received PID does not match this processes'.\n");
		goto close_requests_fifo;
	}

	int num_allocated_seats;
	if (sscanf(buffer + offset, "%d %n", &num_allocated_seats, &ret) < 1) {
		fprintf(stderr, "Unable to parse number of allocated seats\n");
		goto close_requests_fifo;
	}
	offset += ret;
	printf("Allocated seats:\n%s\n\n", buffer + offset);
	int *allocated_seats = malloc(num_allocated_seats * sizeof(*allocated_seats));
	if (!allocated_seats) {
		perror("malloc allocated seats");
		goto close_requests_fifo;
	}

	for (int i = 0, n; i < num_allocated_seats; ++i, offset += n)
		if (sscanf(buffer + offset, "%d %n", &allocated_seats[i], &n) < 1) {
			fprintf(stderr, "Unable to parse allocated seats numbers\n");
			goto free_allocated_seats;
		}

	const char *clog_name = "clog.txt";
	const char *cbook_name = "cbook.txt";
	FILE *clog = fopen(clog_name, "w");
	FILE *cbook = fopen(cbook_name, "w");
	if (!clog || !cbook) {
		perror("fopen");
		goto free_allocated_seats;
	}

	const char *shm_clog_mut_name = "/clog_mut";
	const char *shm_cbook_mut_name = "/cbook_mut";
	int shm_clog = shm_open(shm_clog_mut_name, O_RDWR|O_CREAT, 0666);
	int shm_cbook = shm_open(shm_cbook_mut_name, O_RDWR|O_CREAT, 0666);
	if (shm_clog < 0 || shm_cbook < 0) {
		perror("shm_open clog mutex");
		fprintf(stderr, "Unable to open shared memory for a mutex to write to file, output may be interleaved\n");
	}
	bool clog_use_mut = shm_clog > 0, cbook_use_mut = shm_cbook > 0;
	pthread_mutex_t *clog_shmut, *cbook_shmut;
	if (clog_use_mut) {
		clog_shmut = mmap(NULL, sizeof(*clog_shmut), PROT_READ|PROT_WRITE, MAP_SHARED, shm_clog, 0);
		if (clog_shmut == MAP_FAILED) {
			clog_use_mut = false;
			shm_unlink(shm_clog_mut_name);
		}
	}
	if (cbook_use_mut) {
		cbook_shmut = mmap(NULL, sizeof(*cbook_shmut), PROT_READ|PROT_WRITE, MAP_SHARED, shm_cbook, 0);
		if (cbook_shmut == MAP_FAILED) {
			cbook_use_mut = false;
			shm_unlink(shm_cbook_mut_name);
		}
	}
	pthread_mutexattr_t shmut_attr;
	pthread_mutexattr_init(&shmut_attr);
	pthread_mutexattr_setpshared(&shmut_attr, PTHREAD_PROCESS_SHARED);
	if (clog_use_mut)
		pthread_mutex_init(clog_shmut, &shmut_attr);
	if (cbook_use_mut)
		pthread_mutex_init(cbook_shmut, &shmut_attr);

print_clog:
	for (int i = 0; i < num_allocated_seats; ++i) {
		if (clog_use_mut)
			pthread_mutex_lock(clog_shmut);
		if (ret < 0)
			fprintf(clog, "%0*d %s\n", WIDTH_PID, getpid(), error_codes[-ret]);
		else {
			fprintf(clog, "%0*d ", WIDTH_PID, getpid());
			for (int i = 0; i < num_allocated_seats; ++i)
				fprintf(clog, "%02d.%02d ", allocated_seats[i], num_allocated_seats);
			fprintf(clog, "\n");
		}
		if (clog_use_mut)
			pthread_mutex_unlock(clog_shmut);
	}

	for (int i = 0; i < num_allocated_seats; ++i) {
		if (cbook_use_mut)
			pthread_mutex_lock(cbook_shmut);
		fprintf(cbook, "%0*d\n", WIDTH_SEAT, allocated_seats[i]);
		if (cbook_use_mut)
			pthread_mutex_unlock(cbook_shmut);
	}

	if (clog_use_mut) {
		pthread_mutex_destroy(clog_shmut);
		munmap(clog_shmut, sizeof(*clog_shmut));
	}
	if (cbook_use_mut) {
		pthread_mutex_destroy(cbook_shmut);
		munmap(cbook_shmut, sizeof(*cbook_shmut));
	}
	pthread_mutexattr_destroy(&shmut_attr);
	shm_unlink(shm_clog_mut_name);
	shm_unlink(shm_cbook_mut_name);

// cleanup de todos os recursos criados na execução do programa.
// estão pela ordem inversa à usada para a sua criação; assim, através do goto
// é possível saltar para o ponto necessário e todas as instruções seguintes serão executadas normalmente,
// tornando a operação mais simples.

close_cbook:
	if (fclose(cbook) < 0)
		perror("fclose cbook");

close_clog:
	if (fclose(clog) < 0)
		perror("fclose clog");

free_allocated_seats:
	free(allocated_seats);

close_requests_fifo:
	if (close(requests_fifo) < 0)
		perror("close requests_fifo");

close_fifo:
	if (close(fifo) < 0)
		perror("close fifo");

unlink_fifo:
	if (unlink(fifo_name) < 0)
		perror("unlink fifo");

free_pref_seat_list:
	free(pref_seat_list);

	return 0;
}
