#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

int main() {
    int fd = open("/dev/rgbLeds", O_RDWR);
    int red, green, blue;

    printf("Insert rgb values: ");
    scanf("%d%d%d", &red, &green, &blue);

    uint32_t value = red | (green << 8) | (blue << 16);

    ssize_t write_retval = write(fd, &value, sizeof(uint32_t));

    if (write_retval < 0) {
        printf("There was an error: %s\n", strerror(errno));
    }
    printf("Write Retval: %ld\n", write_retval);
    return 0;
}

