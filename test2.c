#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main()
{
    int fd;
    char buf[6] = {0};

    fd = open("/dev/derchar", O_RDWR);

    write(fd, "HELLOWORLD", 10);

    lseek(fd, 5, SEEK_SET);

    read(fd, buf, 5);

    printf("Read: %s\n", buf);

    close(fd);

    return 0;
}
