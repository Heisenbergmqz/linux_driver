#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

#define DEVICE_PATH "/dev/dev_char0"


int main(void)
{
    int fd;
    struct pollfd fds;
    ssize_t ret;
    int ret1;
    char read_buf[128] = {0};
    fd = open(DEVICE_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    printf("read before\n");

    fds.fd = fd;
    fds.events= POLLIN;
    ret1 = poll(&fds, 1, 10000);
    if(ret1 == 0)
    {
        printf("time out\n");
        close(fd);
        return 0;  
    }

    if(fds.revents & POLLIN)
    {
        ret = read(fd, read_buf, sizeof(read_buf) - 1);
        if (ret < 0) {
            perror("read");
            close(fd);
            return -1;
        }

        /*
        * 防止字符串结尾没有 '\0'
        */
        read_buf[ret] = '\0';

        printf("read success: %zd bytes\n", ret);
        printf("data: \"%s\"\n", read_buf);
    }

    close(fd);

    return 0;
}

