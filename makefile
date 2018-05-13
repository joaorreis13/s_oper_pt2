CC = gcc
CFLAGS = -Wall -Wextra

all: start client server

start: start.c
	$(CC) $(CFLAGS) $^ -o $@

client: client.c
	$(CC) $(CFLAGS) $^ -o $@

server: server.c queue.o ticket_office.o
	$(CC) $(CFLAGS) -pthread $^ -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	-rm -f *.o client server start

clean_logs:
	-rm -f *log.txt *book.txt