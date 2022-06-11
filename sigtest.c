
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int a = -1;

void reset(int sig) {
    printf("%d", a);
}

int prepare() {
    a = 111;
    return 0;
}

int run() {
    return sleep(a);
}

int runall() {
    prepare();
    run();
    return 0;
}

int main(int argc, char* argv[]) {
    signal(SIGCHLD, reset);

    runall();
}