#include "fifo.h"

int create_fifo_request_write(char *name)
{
	//FIFO deve ter o nome ansXXXXX, em que XXXXX representa o PID do cliente (TO-DO)
	int fd;
	char request[14];
    sprintf(request,"/tmp/");
    if(name[0]=='r'){
        sprintf(request+6,"requests");
    }
    else{
        sprintf(request+6,"ans%5zu",(size_t)getpid());
    }
	// create the FIFO (named pipe)
	mkfifo(request, 0666);

	// Write something to fifo
	fd = open(request, O_WRONLY);
	write(fd, "Hi", sizeof("Hi"));
	close(fd);

	// remove the FIFO
	unlink(request);

	return 0;
}