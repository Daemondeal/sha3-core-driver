#include <errno.h>
#include <stdio.h>

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define PROCS_TO_OPEN 9
#define BUFSIZE 512
#define DEVICE_LOCATION "/dev/ketchup_driver"

void child(int start_fd, int output_fd, int is_blocking) {
    char buffer[BUFSIZE] = {0};
    char digest[64] = {0};
    read(start_fd, buffer, 1);

    int flags = (is_blocking ? 0 : O_NONBLOCK) | O_RDWR;

    int fd = open(DEVICE_LOCATION, flags);

    if (fd < 0) {
        sprintf(buffer, "Error: (%d) (%d) %s", fd, errno, strerror(errno));
        write(output_fd, buffer, BUFSIZE);
        return;
    }

    sleep(1);

    write(fd, "Hello World!", strlen("Hello World!"));
    int diglen = read(fd, digest, 64);

    sprintf(buffer, "len = %d, digest[0] = %02x", diglen, digest[0]);
    write(output_fd, buffer, strlen(buffer));

    close(fd);
}

int main(int argc, char *argv[]) {
    int start_pipe[2], tmp_pipe[2];
    int output_pipes[PROCS_TO_OPEN];
    char dummy[PROCS_TO_OPEN] = {0};
    pid_t pid, pids[PROCS_TO_OPEN];
    char buffer[BUFSIZE];

    int start_fd, is_blocking;

    is_blocking = argc < 2 || (strcmp(argv[1], "n") != 0);

    printf("Starting processess... (blocking = %d)\n", is_blocking);

    pipe(start_pipe);
    start_fd = start_pipe[1];

    for (int i = 0; i < PROCS_TO_OPEN; i++) {
        pipe(tmp_pipe);
        pid = fork();
        if (pid == 0) {
            child(start_pipe[0], tmp_pipe[1], is_blocking);
            return 0;
        }
        pids[i] = pid;
        output_pipes[i] = tmp_pipe[0];
    }

    printf("Procs started! Giving the go signal\n");
    write(start_fd, dummy, PROCS_TO_OPEN);


    for (int i = 0; i < PROCS_TO_OPEN; i++) {
        wait(NULL);
    }


    printf("All done! Outputs: \n");
    for (int i = 0; i < PROCS_TO_OPEN; i++) {
        memset(buffer, 0, BUFSIZE);

        read(output_pipes[i], buffer, BUFSIZE - 1);
        printf("%d: %s\n", pids[i], buffer);
    }
}
