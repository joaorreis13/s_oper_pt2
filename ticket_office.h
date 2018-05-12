#pragma once

#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define DELAY() sleep(1)

typedef struct seat {
	pthread_mutex_t mut;
	int clientId;
	bool free;
} Seat;

int isSeatFree(Seat *seats, int seatNum);
void bookSeat(Seat *seats, int seatNum, int clientId);
void freeSeat(Seat *seats, int seatNum);
