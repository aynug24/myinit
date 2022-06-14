#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "log/log.h"
#include "run_daemon/run_daemon.h"

const char* DEV_NULL = "/dev/null";

void ignore_terminal_sigs() { // while we can just exit, funcs can be void
    if (getppid() != 1) {
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
    }
}

void flush_file_bufs() {
    fflush(stdout); // flush bufs before fork
    fflush(stderr);
    flush_log();
}

void fork_daemon() {
    pid_t fork_pid = fork();
    if (fork_pid < 0) {
        const char* msg = "Couldn't fork daemon starting process";
        perror(msg);
        log_crit_e(msg, errno);

        exit(-1);
    }
    if (fork_pid > 0) {
        printf("%d", fork_pid);
        exit(0);
    }
}

FILE* ensure_open_log() {
    FILE* log_file = open_log();
    if (log_file == NULL) {
        perror("Couldn't open log file");
        exit(-1);
    }
    return log_file;
}

pid_t create_session() {
    pid_t sid = setsid();
    if (sid < 0) {
        log_crit_e("Couldn't create new session", errno);
        exit(-1);
    }
    return sid;
}

void close_std_fds() {
    if (close(STDIN_FILENO) < 0
        || close(STDOUT_FILENO) < 0
        || close (STDERR_FILENO) < 0) {

        log_crit_e("Error closing file descriptors", errno);
        exit(-1);
    }
}

void open_low_fds_to_devnull() {
    if (open(DEV_NULL, O_RDONLY) < 0
        || open(DEV_NULL, O_WRONLY) < 0
        || open(DEV_NULL, O_WRONLY) < 0) {

        log_crit_e("Error opening lowest three fds to /dev/null", errno);
        exit(-1);
    }
}

void ensure_chdir_root() {
    if (chdir("/") < 0) {
        log_crit_e("Error changing working directory to root", errno);
        exit(-1);
    }
}

void create_daemon() {
    ignore_terminal_sigs();

    ensure_open_log(); // open log early so that we can see forked messages

    flush_file_bufs();

    fork_daemon();

    umask(0);

    create_session();

    close_std_fds(); // closing all fds will close log; also we don't have any other files open

    open_low_fds_to_devnull(); // so that code that uses 0-2 fds doesn't fail or use user fds

    ensure_chdir_root();
}

int main(int argc, char* argv[]) {
    create_daemon();
    int res = run_daemon(argc, argv);
    close_log();
    return res;
}