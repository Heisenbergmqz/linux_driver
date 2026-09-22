#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include "my_ioctl.h"
int main(void)
{
    int fd;
    int value;

    fd = open("/dev/dev_char0", O_RDWR);

    value = 100;

    ioctl(fd, SET_VALUE, &value);

    value = 0;

    ioctl(fd, GET_VALUE, &value);

    printf("value = %d\n", value);

    ioctl(fd, CLEAR);

    close(fd);
    return 0;
}