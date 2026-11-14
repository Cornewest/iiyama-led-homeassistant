#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 3) return 1;
    int cmd = atoi(argv[1]);
    int val = atoi(argv[2]);
    int fd = open("/dev/led_gpio", O_RDWR);
    if (fd < 0) return 2;
    ioctl(fd, cmd, val);
    close(fd);
    return 0;
}
