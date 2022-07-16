#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>

int main(int argc, char *argv[])
{
    int fd, ret;
    int key_val;
    struct input_event ev;

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

    printf("key0 val = %d\n", ev.value);
    for(;;)
    {
        ret = read(fd, &ev, sizeof(struct input_event));
       // printf("code %d value %d \n", ev.code, ev.code);
        //if (ret)
        {
            switch (ev.type)
            {
            case EV_KEY:
                if (KEY_1 == ev.code)
                    printf("key0 val = %d\n", ev.value);
                break;
            
            default:
                break;
            }
        }
       // sleep(1);
    }
    printf("stop\n");
    close(fd);
}
