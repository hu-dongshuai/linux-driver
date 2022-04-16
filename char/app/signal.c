#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

void input_handler(int sig)
{
	printf("input available\n" );
}

int main()
{
	int oflags;
	int fd;
	fd = open("/dev/test", O_RDWR, S_IRUSR | S_IWUSR);
	if(fd == -1)
	{
		printf("failed to open\n");
	}

	printf("signal start\n");
	signal(SIGIO, input_handler);
	fcntl(fd, F_SETOWN, getpid());
	oflags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, oflags | FASYNC);
	while(1)
	{
		sleep(100);
	}
}
