#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

#define DEVICE_PATH "/dev/dev_char0"


int main(void)
{
    int fd;
 
    ssize_t ret;
    int count = 0;
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) 
    {
        perror("open");
        return -1;
    }
    while(1)
    {
        ret = read(fd, &count, sizeof(count));
        if (ret < 0) 
        {
            perror("read");
            close(fd);
            return -1;
        }
        printf("data:%d\n", count);
        sleep(1);
    }

    close(fd);

    return 0;
}

