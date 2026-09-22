#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define DEVICE_PATH "/dev/dev_char0"

int main(void)
{
    int fd;
    ssize_t ret;
    char write_buf[] = "hello";

    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    ret = write(fd, write_buf, strlen(write_buf));
    if (ret < 0) {
        perror("write");
        close(fd);
        return -1;
    }

    printf("write success: %zd bytes\n", ret);

    close(fd);

    return 0;
}
