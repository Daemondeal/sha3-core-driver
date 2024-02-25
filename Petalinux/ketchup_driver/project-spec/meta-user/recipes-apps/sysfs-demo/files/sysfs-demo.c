#include <bits/types/sigset_t.h>
#include <errno.h>
#include <stdio.h>

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "ketchup_lib/ketchup_lib.h"

#define PROCS_TO_OPEN 3

void signal_handler(int signo) {
    // do nothing
}

void child(int start_fd, int proc_idx) {
    kc_sha3_context context;
    char dummy;

    if ((proc_idx % 4) == 0) {
        kc_sha3_224_init(&context);
    } else if ((proc_idx % 4) == 1) {
        kc_sha3_256_init(&context);
    } else if ((proc_idx % 4) == 2) {
        kc_sha3_384_init(&context);
    } else {
        kc_sha3_512_init(&context);
    }

    read(start_fd, &dummy, 1);

    kc_sha3_close(&context);
}

int main(int argc, char *argv[])
{
    int start_pipe[2];
    sigset_t sigset;
    int sig;
    char buffer[PROCS_TO_OPEN] = {0};

    pipe(start_pipe);

    for (int i = 0; i < PROCS_TO_OPEN; i++) {
        int pid = fork();
        if (pid == 0) {
            close(start_pipe[1]);
            child(start_pipe[0], i);
            return 0;
        }
    }

    close(start_pipe[0]);

    printf("All the processes have been started.\n");
    printf("Run \"kill -USR1 %d\" to resume them.\n", getpid());


    signal(SIGUSR1, signal_handler);
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);

    sigwait(&sigset, &sig);

    write(start_pipe[1], buffer, PROCS_TO_OPEN);
    for (int i = 0; i < PROCS_TO_OPEN; i++) {
        wait(NULL);
    }

    printf("All done!");

    return 0;
}
