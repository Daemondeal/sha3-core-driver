#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

int main() {
    char buffer[255] = {0};
    int fd = open("/dev/example_module", O_RDONLY);
    if (fd < 0) {
        printf("Failed to open file: %s\n", strerror(errno));
        return fd;
    }

    ssize_t bytes_read = read(fd, buffer, 5);

    int result = ioctl(fd, 0x01);

    close(fd);

    if (bytes_read < 0) {
        printf("Error reading device: %s\n", strerror(errno));
        return bytes_read;
    }
    printf("Successfully read %ld characters from the device: \"%s\"\n", bytes_read, buffer);
    printf("Result of ioctl: %d\n", result);

    int params = _IOW('a', 'b', uint32_t);

    printf("ioctl params: %d\n", params);
    

    return 0;
}