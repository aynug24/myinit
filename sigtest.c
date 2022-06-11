
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

int a = -1;

void reset(int sig) {
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


int main(int argc, char* argv[]) {
    printf0("%d %s", 5, "my_s");
}