#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#define MAX_ROOM_SEATS			9999
#define MAX_CLI_SEATS			99
#define WIDTH_PID				5
#define WIDTH_XXNN				5
#define WIDTH_SEAT				4

#define BUFFER_SIZE				4096

int main(int argc, const char *argv[]) {

	if (argc < 4) {
		printf("Usage: %s <timeout> <num_wanted_seats> <pref_seat_list>\n", *argv);
		exit(EXIT_FAILURE);
	}

	int timeout, num_wanted_seats, *pref_seat_list = NULL;
	if (sscanf(argv[1], "%d", &timeout) < 1 || timeout < 0) {
		printf("Invalid timeout\n");
		exit(EXIT_FAILURE);
	}
	if (sscanf(argv[2], "%d", &num_wanted_seats) < 1 || num_wanted_seats < 0) {
		printf("Invalid number of wanted seats\n");
		exit(EXIT_FAILURE);
	}
	printf("Timeout is %u\nNumber of wanted seats is %u\n", timeout, num_wanted_seats);

	int num_pref_seat = 0;
	for (int c = 0, offset = 0, n, c_size = 0, size = strlen(argv[3]); offset < size; offset += c) {
		if (sscanf(argv[3] + offset, "%d%n", &n, &c) < 1) {
			int *tmp = realloc(pref_seat_list, num_pref_seat * sizeof(*pref_seat_list));
			if (!tmp) {
				perror("realloc");
				break;
			}
			pref_seat_list = tmp;
			break;
		}
		if (n < 0) {
			continue;
		}
		if (num_pref_seat == c_size) {
			c_size = c_size? c_size *= 2: 1;
			int *tmp = realloc(pref_seat_list, c_size * sizeof(*pref_seat_list));
			if (!tmp) {
				perror("realloc");
				break;
			}
			pref_seat_list = tmp;
		}
		pref_seat_list[num_pref_seat++] = n;
	}

	if (num_pref_seat < num_wanted_seats) {
		printf("Insuficient number of seat preferences\n");
		// free pref_seat_list
		return 0;
	}
	for (int i = 0; i < num_pref_seat; ++i)
		printf("%d ", pref_seat_list[i]);
	printf("\n");

	char fifo_name[14] = "/tmp/ans";
	snprintf(fifo_name + 8, WIDTH_PID + 1, "%05d", getpid());
	if (mkfifo(fifo_name, 0666) < 0) {
		perror("mkfifo");
		// free pref_seat_list
		return 0;
	}
	int fifo = open(fifo_name, O_RDONLY|O_NONBLOCK);
	if (fifo < 0) {
		perror("fifo open");
		// unlink fifo
		// free pref_seat_list
		return 0;
	}

	char buffer[BUFFER_SIZE];
	int n = 0;
	snprintf(buffer + n, BUFFER_SIZE - n, "%d %d %n", getpid(), num_wanted_seats, &n);
	for (int i = 0, c; i < num_pref_seat; ++i, n += c)
		snprintf(buffer + n, BUFFER_SIZE - n, "%d %n", pref_seat_list[i], &c);
	strcpy(&buffer[n - 1], "\n");
	printf("buffer: %s\n", buffer);

	int requests_fifo = open("/tmp/requests", O_WRONLY);
	if (requests_fifo < 0) {
		perror("requests fifo open");
		// close fifo
		// unlink fifo
		// free pref_seat_list
		return 0;
	}
	if (write(requests_fifo, buffer, n) < 0) {
		perror("requests fifo write");
		// close requests_fifo
		// close fifo
		// unlink fifo
		// free pref_seat_list
		return 0;
	}

	struct pollfd pfd = { .fd = fifo, .events = POLLIN };
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
	}
	/*if (poll(&pfd, 1, 1000 * timeout) < 0) {
		perror("poll");
		// close requests_fifo
		// close fifo
		// unlink fifo
		// free pref_seat_list
		return 0;
	}
	if (pfd.revents & POLLHUP) {
		printf("timeout\nExiting...\n");
		// close requests_fifo
		// close fifo
		// unlink fifo
		// free pref_seat_list
		return 0;
	}
	if (pfd.revents & (POLLERR|POLLNVAL)) {
		printf("poll error\nExiting...\n");
		// close requests_fifo
		// close fifo
		// unlink fifo
		// free pref_seat_list
		return 0;
	}*/

	/*if (pfd.revents & POLLIN)
		printf("msg received\n");
	printf("revents: %#010x\n", pfd.revents);*/



	// close requests_fifo
	// close fifo
	// unlink fifo
	// free pref_seat_list
	return 0;
}
