#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include "bilheteira.h"


int isSeatFree(Seat *seats, int seatNum){
    if (seats[seatNum].booked==1 && seats[seatNum].pid_client!=0){
      DELAY();
      return 0;}
    else{
        DELAY();
        return 1;
    }
}

void bookSeat(Seat *seats, int seatNum, int clientId){
    DELAY();
    seats[seatNum].booked=1;
    seats[seatNum].pid_client=clientId;
}

void freeSeat(Seat *seats, int seatNum){
  DELAY();
  seats[seatNum].booked=0;
  seats[seatNum].pid_client=0;
}


//so para testar
int main(){
  Seat seats[5];
  printf("%d\n", isSeatFree(seats, 1));
  bookSeat(seats, 1, 123);
  printf("%d %d\n", seats[1].booked, seats[1].pid_client);
  freeSeat(seats, 1);
  printf("%d %d\n", seats[1].booked, seats[1].pid_client);
  return 0;
}
