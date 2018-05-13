#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define WIDTH_PID		5
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

	//interpreta a mensagem do servidor (adicionar timeout)
	char serv_answer[] = read(fifo, buffer, BUFFER_SIZE - 1);
	

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

	// ler do FIFO de resposta com um timeout
	/*struct pollfd pfd = { .fd = fifo, .events = POLLIN };
	int ret = poll(&pfd, 1, 1000 * timeout);
	switch (ret) {
		case -1:
			perror("poll");
			break;
		case 0:
			printf("timeout. exiting...\n");
			break;
		default:
			printf("msg received: \n");
			read(fifo, buffer, BUFFER_SIZE);
			printf("'%s'\n", buffer);
	}*/

	// leitura sem timeout
	n = read(fifo, buffer, BUFFER_SIZE - 1);
	if (n < 0) {
		perror("read fifo");
		goto close_requests_fifo;
	} printf("read %d bytes - '%.*s' from fifo\n", n, n, buffer);

	/*
	 * Em falta:
	 * 	Interpretar a mensagem enviada pelo servidor.
	 * 	Escrever no ficheiro clog.txt o estado da operação. Ficheiro (FILE *) ainda não criado/aberto neste ponto
	 * 	Escrever no ficheiro cbook.txt os identificadores dos lugares reservados. Ficheiro ainda não criado/aberto neste ponto.
	 * 
	 * 	Nota: Cuidado na escrita nestes dois ficheiros uma vez que todos os clientes estarão concurrentemente a escrever nele.
	 * 	Para assegurar uma escrita correcta, o ficheiro deve ser aberto (ou criado) em modo "append",
	 * 	sendo que o fprintf é line-buffered
	 * 	Deste modo, cada linha será escrita no ficheiro por cada processo de forma atómica, i.e.,
	 * 	não será interompida por outros processos a escrever no mesmo ficheiro
	 */

// cleanup de todos os recursos criados na execução do programa.
// estão pela ordem inversa à usada para a sua criação; assim, através do goto
// é possível saltar para o ponto necessário e todas as instruções seguintes serão executadas normalmente,
// tornando a operação mais simples.

// falta adicionar o fecho dos ficheiros mencionados acima.

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
