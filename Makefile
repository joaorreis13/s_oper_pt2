CC = gcc
CFLAGS = -Wall -Wextra -g
EXE_DIR_NAME = T2G05

all: client server

start: start.c
	$(CC) $(CFLAGS) $^ -o $@

client: client.c
	-mkdir -p $(EXE_DIR_NAME)
	$(CC) $(CFLAGS) -pthread $^ -o $(EXE_DIR_NAME)/$@ -lrt

server: server.c queue.o ticket_office.o
	-mkdir -p $(EXE_DIR_NAME)
	$(CC) $(CFLAGS) -pthread $^ -o $(EXE_DIR_NAME)/$@

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	-rm -f *.o $(EXE_DIR_NAME)/client $(EXE_DIR_NAME)/server

clean_logs:
	-rm -f $(EXE_DIR_NAME)/*log.txt $(EXE_DIR_NAME)/*book.txt
