#ifndef BILHETEIRA_H
#define BILHETEIRA_H

#define DELAY() sleep(1)

int isSeatFree(Seat *seats, int seatNum);
void bookSeat(Seat *seats, int seatNum, int clientId);
void freeSeat(Seat *seats, int seatNum);

#endif
