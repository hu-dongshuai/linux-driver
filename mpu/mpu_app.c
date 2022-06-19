#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main()
{
	short resive_data[6]; 
	int error;
	int fd = open("/dev/mpu_dev", O_RDWR);
	if (fd<0)
	{
		printf("/dev/mpu_dev open failed\n");
		return -1;
	}

  while(1)
  {
	 int error = read(fd,resive_data,12);
    if(error < 0)
    {
        printf("write file error! \n");
        close(fd);
        /*判断是否关闭成功*/
    }

    /*打印数据*/
    printf("AX=%d, AY=%d, AZ=%d ",(int)resive_data[0],(int)resive_data[1],(int)resive_data[2]);
	printf("    GX=%d, GY=%d, GZ=%d \n \n",(int)resive_data[3],(int)resive_data[4],(int)resive_data[5]);
	sleep(1);

  }

    /*关闭文件*/
    error = close(fd);
    if(error < 0)
    {
        printf("close file error! \n");
    }
    
    return 0;
}
