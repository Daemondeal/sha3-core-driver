#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

int main(int argc, char const *argv[])
{
    /* code */
    int fd = open("/dev/axi4adder", O_RDWR); // apro il file
    int reg1, reg2;
    printf("Insert two values to add: ");
    scanf("%d%d", &reg1, &reg2);
    // Ora devo convertire queste variabili in due write
    uint32_t value = red | (green << 8) | (blue << 16);
    ssize_t write_retval = write(fd, &value, sizeof(uint32_t));
    if (write_retval < 0) {
        printf("There was an error: %s\n", strerror(errno));
    }
    printf("Write Retval: %ld\n", write_retval);
    return 0;
    return 0;
}
