#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd, ret;
    int key_val;

    if (2 != argc)
    {
        printf("Usage:\n"
            "\t./keyApp /dev/key\n");
    }

    fd=open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }

    for(;;)
    {
        read(fd, &key_val, sizeof(int));
        printf("key val %d\n", key_val);
        if (0 == key_val)
            printf("key press\n");
        else if(1 == key_val)
            printf("key release\n");
    }
    close(fd);
}
