
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

int a = -1;

void set_restart(int sig) {
    printf("%d\n", a);
    //fflush(stdout);
}

void log_sigsegv(int sig) {
    char* name = strsignal(sig);
    printf("%s\n", name);
    exit(128 + sig);
}

int prepare() {
    a = 111;
    return 0;
}

int run() {
    while (1)
        sleep(a);
}

int runall() {
    prepare();
    run();
    return 0;
}

void printf0(const char* format, ... ) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end( args );
}

volatile sig_atomic_t seen_signal = 0;

void sig(int sig) {
    seen_signal = 1;
}


int main(int argc, char* argv[]) {

    signal(SIGINT, sig);

    printf("sig is %d\n", seen_signal);

    pause();

    printf0("sig is %d\n", seen_signal);
}