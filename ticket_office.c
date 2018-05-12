#include "ticket_office.h"

int isSeatFree(Seat *seats, int seatNum) {
	if (!seats)
		return -1;
	pthread_mutex_lock(&seats[seatNum].mut);
	DELAY();
	if (seats[seatNum].free)
		return 1;
	else
		pthread_mutex_unlock(&seats[seatNum].mut);
	return 0;
}

void bookSeat(Seat *seats, int seatNum, int clientId) {
	if (!seats)
		return;
	pthread_mutex_lock(&seats[seatNum].mut);
	if (!seats[seatNum].free)
		return;
	seats[seatNum].free = false;
	seats[seatNum].clientId = clientId;
	DELAY();
	pthread_mutex_unlock(&seats[seatNum].mut);
	return;
}

void freeSeat(Seat *seats, int seatNum) {
	if (!seats)
		return;
	pthread_mutex_lock(&seats[seatNum].mut);
	seats[seatNum].free = true;
	seats[seatNum].clientId = 0;
	DELAY();
	pthread_mutex_unlock(&seats[seatNum].mut);
	return;
}
