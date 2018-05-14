CC = clang
CFLAGS = -Wall -Wextra -g
DEBUG_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer

all: start client server

start: start.c
	$(CC) $(CFLAGS) $^ -o $@

client: client.c
	$(CC) $(CFLAGS) -pthread $^ -o $@ -lrt

server: server.c queue.o ticket_office.o
	$(CC) $(CFLAGS) -pthread $^ -o $@

debug: CFLAGS += $(DEBUG_FLAGS)
debug: start client server

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	-rm -f *.o client server start
	make clean_logs

clean_logs:
	-rm -f *log.txt *book.txt
