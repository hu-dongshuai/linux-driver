#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

int main(int argc, char *argv[])
{
    fd_set readfds;
    int fd, ret;
    int key_val;

    if (2 != argc)
    {
        printf("Usage:\n"
            "\t./keyApp /dev/key\n");
    }

    fd=open(argv[1], O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        return -1;
    }

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    for(;;)
    {
       ret = select(fd + 1, &readfds, NULL, NULL, NULL);       
       printf("select \n");
       switch (ret)
       {
       case 0:
        break;
       case -1:
       default:
        if (FD_ISSET(fd, &readfds)){
            read(fd, &key_val, sizeof(int));
            if (0 == key_val)
                printf("key press\n");
            else if(1 == key_val)
                printf("key release\n");
        }
        break;
       }

    }
    close(fd);
    return 0;
}
