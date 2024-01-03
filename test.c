#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
 
#define WR_PERIPH_HASH_SIZE _IOW('ketchup', 1, uint32_t*)
#define RD_PERIPH_HASH_SIZE _IOR('ketchup', 2, uint32_t*)
int main()
{
        int fd;
        uint32_t value, number;
        printf("*********************************\n");
        printf("*******ketchup_driver_test*******\n");
 
        printf("\nOpening Driver\n");
        fd = open("/dev/ketchup_driver", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }
        printf("Testing the ioctl attributes\n");
        while (1)
        {
            printf("Enter the wanted hash size [0:512, 1:384 2:256 3:224]\n");
            scanf("%d",&number);
            printf("Writing Value to Driver\n");
            ioctl(fd, WR_PERIPH_HASH_SIZE, (uint32_t*) &number); 
 
            printf("Reading the hash size from the driver\n");
            ioctl(fd, RD_PERIPH_HASH_SIZE, (uint32_t*) &value);
            printf("Value is %d\n", value);
        }
 
        // How we want to suffer
        int err = write(fd, "Hello", 5);
        if (err < 0){
            printf("sad face\n");
        }
        char *digest[512/8] = {0};
        read(fd, digest, 512/8);
        for (int i = 0; digest[i] != '\0'; i++) {
            printf("%02x", digest[i]);
        }
        printf("\n");
        printf("Closing Driver\n");
        close(fd);
}