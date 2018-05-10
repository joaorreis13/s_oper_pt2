#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



#define MAX_ROOM_SEATS 9999
#define MAX_CLI_SEATS 99
#define WIDTH_SEAT 4
#define MAX_BUF 1024
#define MAX_TICKET_OFFICES 5
#define MAX_OPEN_TIME 1000

typedef struct{
  pid_t pid_client;
  int booked ; //boolean
} Seat ;


int main_thread();

//Function that creates bilheteira threads
int threads_bilheteira();

//FIFO that receives the seat reservation requests
int create_fifo_request_read();

//FIFO that sends the answer to the client
int create_fifo_resposta_write();


#endif
