#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
int fds[2];
struct termios t_save;
uint8_t to_send[4096];
int to_send_len = 0;
int can_uart_write = 0;
void handler(int arg)
{
	tcsetattr(fds[1], TCSADRAIN,&t_save) ;
	close(fds[0]);
	close(fds[1]);
	printf("close files");
	exit(0);
}
// 打开串口，配置为115200, 8N1, 无流控
int open_uart(const char* dev)
{
    struct termios options;
	int ret;
    //清空所有属性
    memset(&options, 0, sizeof(options));
    //设置输入输出速率为115200
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    //8个数据位
    options.c_cflag |= CS8;
    //无校验位（其实这句可以不写，因为已经全部清0，只需要设置为1的位）
    options.c_cflag &= ~PARENB;
    //1个停止位（其实这句可以不写，因为已经全部清0，只需要设置为1的位）
    options.c_cflag &= ~CSTOPB;
    //超时时间为0
    options.c_cc[VTIME] = 0;
    //每次接收长度为1个字节
    options.c_cc[VMIN] = 1;
    // 可能的错误信息
    char error[1024];
    // 打开设备
    int fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0)
    {
        sprintf(error, "<eth_uart> open('%s') failed!\n", dev);
        goto err;
    }
    // 设置属性

    tcgetattr(fd, &t_save) ;
	ret = tcsetattr(fd, TCSADRAIN, &options) ;
    if(ret != 0)
    {
        strcpy(error, "<eth_uart> tcsetattr() failed!\n");
		printf("ret = %d\n", ret);
        goto err;
    }
    // 清空缓存
    if(tcflush(fd, TCIOFLUSH) != 0)
    {
        strcpy(error, "<eth_uart> tcflush() failed!\n");
        goto err;
    }
    return fd;
    // 错误处理
    err:
        if(fd > 0)
            close(fd);
        printf("%s", error);
        return -1;
}

// 初始化网卡IO口和串口
int init(const char* eth_uio, const char* uart_dev, int* fds)
{
    // 可能的错误信息
    char error[1024];
    // 打开网卡输入输出口
    int eth_fd = open(eth_uio, O_RDWR);
    if(eth_fd < 0)
    {
        sprintf(error, "<eth_uart> open('%s') failed!\n", eth_uio);
        goto err_1;
    }
    // 设置为非阻塞
    if(fcntl(eth_fd, F_SETFL, fcntl(eth_fd, F_GETFL, 0) | O_NONBLOCK) < 0)
    {
        strcpy(error, "<eth_uart> fcntl(eth, O_NONBLOCK) failed!\n");
        goto err_2;
    }
    // 打开串口
    int uart_fd = open_uart(uart_dev);
    if(uart_fd < 0)
    {
        // 错误信息在open_uart()中已经输出
        strcpy(error, "");
        goto err_2;
    }
    // 两个文件描述符，传出
    fds[0] = eth_fd;
    fds[1] = uart_fd;
    return 0;
    // 错误处理
    err_2:
        close(eth_fd);
    err_1:
        ;
        printf("%s", error);
        return -1;
}

int main_routine(int eth_fd, int uart_fd, int max_buf)
{
    // 可能的错误信息
    char error[1024];
    // 要通过串口发出的字节流
    // 要通过串口发出的字节流的长度
    // 已经通过串口发出的长度
    int sent_len = 0;
    // 正在通过串口接收的帧
    uint8_t recving[max_buf];
    // 已经接收的帧长度
    int recving_len = 0;
    // 下一个接收到的字节是否需要转义
    int is_escape = 0;

    while(1)
    {
        // 标记“网卡可读”，“串口可读”，“串口可写”
        int can_eth_read = 0, can_uart_read = 0 ;
        // 没有需要通过串口发送的字节流
        if(to_send_len == 0)
        {
            // 等待可读的文件描述符集合
            fd_set rds;
            FD_ZERO(&rds);
            FD_SET(eth_fd, &rds);
            FD_SET(uart_fd, &rds);
            // select等待
            if(select((eth_fd > uart_fd ? eth_fd : uart_fd) + 1, &rds, 0, 0, 0) < 0)
            {
                strcpy(error, "<eth_uart> select(eth + uart, READ) error!\n");
                goto err;
            }
            can_eth_read = FD_ISSET(eth_fd, &rds);
            can_uart_read = FD_ISSET(uart_fd, &rds);
        }
        // 有需要通过串口发送的字节流
        else
        {
            // 等待可读的文件描述符集合
            fd_set rds;
            FD_ZERO(&rds);
            FD_SET(uart_fd, &rds);
            // 等待可写的文件描述符集合
            fd_set wrs;
            FD_ZERO(&wrs);
            FD_SET(uart_fd, &wrs);
            // select等待
            if(select(uart_fd + 1, &rds, &wrs, 0, 0) < 0)
            {
                strcpy(error, "<eth_uart> select(uart, READ + WRITE) error!\n");
                goto err;
            }
            can_uart_read = FD_ISSET(uart_fd, &rds);
            can_uart_write = FD_ISSET(uart_fd, &wrs);
        }
        // 如果网卡可读
        if(can_eth_read)
        {
            uint8_t frame[max_buf];
            // 从网卡读一个帧
            int len = read(eth_fd, frame, max_buf);
			printf("read from eth %s\n", frame);
            // 避免poll机制的误判
            if(len == 0)
                continue;
            if(len < 0)
            {
                sprintf(error, "<eth_uart> read(eth) == %d!\n", len);
                goto err;
            }
            else if(len > max_buf / 2)
            {
                sprintf(error, "<eth_uart> read(eth) == %d, too long!\n", len);
                goto err;
            }
            // 编码成串口上发送的字节流
            // 开头有START指令，表达为 255,0
            to_send[to_send_len++] = 255;
            to_send[to_send_len++] = 0;
            // 原始帧中的255编码为 255,255，其他不变
            for(int i = 0; i < len; i++)
            {
                to_send[to_send_len++] = frame[i];
                if(frame[i] == 255)
                    to_send[to_send_len++] = 255;
            }
            // 结尾有END指令，表达为 255,1
            to_send[to_send_len++] = 255;
            to_send[to_send_len++] = 1;
        }
        // 如果串口可读
        else if(can_uart_read)
        {
            // 经过编码的字节流
            uint8_t encoded[max_buf];
            // 从串口读
            int len = read(uart_fd, encoded, max_buf);
			printf("read from uart %s\n", encoded);
            if(len < 0)
            {
                sprintf(error, "<eth_uart> read(uart) == %d!\n", len);
                goto err;
            }
            // 依次处理每个字节
            for(int i = 0; i < len; i++)
            {
                uint8_t abyte = encoded[i];
                // 如果当前是转义状态
                if(is_escape)
                {
                    // 遇到了 255,0，表示START指令，则清空之前的数据
                    if(abyte == 0)
                        recving_len = 0;
                    // 遇到了 255,1，表示END指令，则说明接收完了一个数据包
                    else if(abyte == 1)
                    {
                        // 解码后的帧传给网卡
                        if(write(eth_fd, recving, recving_len) != recving_len)
                        {
                            sprintf(error, "<eth_uart> write(eth, %d) != %d!\n", recving_len, recving_len);
                            goto err;
                        }
			            printf("write to eth %s\n", recving);
                    }
                    else
                        recving[recving_len++] = abyte;
                    // 退出转义状态
                    is_escape = 0;
                }
                // 不是转义状态
                else
                {
                    // 遇到了 255，说明接下来的字节是转义状态
                    if(encoded[i] == 255)
                        is_escape = 1;
                    else
                        recving[recving_len++] = abyte;
                }
                if(recving_len == max_buf)
                {
                    strcpy(error, "<eth_uart> too big frame!\n");
                    goto err;
                }
            }
        }
        // 如果串口可写
        else if(can_uart_write)
        {
            //尝试向串口写出
            int sz = write(uart_fd, to_send + sent_len, to_send_len - sent_len);
			printf("write to uart %s\n", to_send);
            if(sz <= 0)
            {
                sprintf(error, "<eth_uart> write(uart) == %d!\n", sz);
                goto err;
            }
            sent_len += sz;
            // 全部发出
            if(sent_len == to_send_len)
            {
                to_send_len = 0;
                sent_len = 0;
            }
        }
    }
    err:
        printf("%s", error);
        handler(1);

        return -1;
}
void *read_uart(void *arg)
{
	int eth_fd=fds[0], uart_fd=fds[1];
	int max_buf = 4096;
    uint8_t recving[max_buf];
    // 已经接收的帧长度
    int recving_len = 0;
     int is_escape = 0;
	printf("read from uart \n");
	while(1)
	{
         // 经过编码的字节流
            uint8_t encoded[max_buf];
            // 从串口读
            int len = read(uart_fd, encoded, max_buf);
            if(len <= 0 || can_uart_write==1 )
            {
				continue;
            }
	        printf("read from uart %s\n",encoded);
            // 依次处理每个字节
            for(int i = 0; i < len; i++)
            {
                uint8_t abyte = encoded[i];
                // 如果当前是转义状态
                if(is_escape)
                {
                    // 遇到了 255,0，表示START指令，则清空之前的数据
                    if(abyte == 0)
                        recving_len = 0;
                    // 遇到了 255,1，表示END指令，则说明接收完了一个数据包
                    else if(abyte == 1)
                    {
                        // 解码后的帧传给网卡
                        if(write(eth_fd, recving, recving_len) != recving_len)
                        {
                        }
			            printf("write to eth %s\n", recving);
                    }
                    else
                        recving[recving_len++] = abyte;
                    // 退出转义状态
                    is_escape = 0;
                }
                // 不是转义状态
                else
                {
                    // 遇到了 255，说明接下来的字节是转义状态
                    if(encoded[i] == 255)
                        is_escape = 1;
                    else
                        recving[recving_len++] = abyte;
                }
                if(recving_len == max_buf)
                {
                }
            }
			sleep(0.1);
	}
}

int main(int argc, char *argv[])
{
    // 两个文件描述符，分别是网卡IO口和串口
    struct sigaction sig;
	pthread_t pid;

    pthread_create(&pid, NULL, &read_uart, NULL);
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = 0;
	sig.sa_handler = handler;
    char *uart = (char *)calloc(1, 20);
    if (argc <= 1)
    {
        memcpy(uart, "/dev/ttyS1", 10);
    } else {
        memcpy(uart, argv[1], strlen(argv[1]));
    }
    if(init("/proc/eth_uart/uio", uart, fds) != 0)
        return -1;
	sigaction(SIGINT, &sig, NULL);
    free(uart);
    if(main_routine(fds[0], fds[1], 4096) == -1)
        return -1;
    // 关闭文件
    close(fds[0]);
    close(fds[1]);
    return 0;
}
