#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "run_daemon.h"
#include "../parse/parse_config.h"
#include "../parse/parse_myinit_args.h"
#include "../log/log.h"
#include "../lists/add.h"

#define WAIT_FOR_CHILDREN_TO_TERMINATE_MS 500

char* config_path;

InitTasks tasks;
pid_t* pids;
bool started_tasks = false;

int log_status(pid_t pid, int status) {
    if (WIFEXITED(status)) {
        return log_info_f("Child process with pid=%d exited with code %d", pid, WEXITSTATUS(status));
    }
    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
#ifdef WCOREDUMP
        return log_info_f(
                "Child process with pid=%d was terminated by signal %d (%s), core was %sdumped",
                pid, sig, strsignal(sig), WCOREDUMP(status) ? "" : "not ");
#else
        return log_info_f(
                "Child process with pid=%d was terminated by signal %d (%s)",
                pid, sig, strsignal(sig));
#endif // WCOREDUMP
    }
    if (WIFSTOPPED(status)) {
        return log_info_f("Child process with pid=%d was stopped by signal %d (%s)", pid, WSTOPSIG(status));
    }
#ifdef WIFCONTINUED // why not
    if (WIFCONTINUED(status)) {
        return log_info_f("Child process with pid=%d was resumed", pid);
    }
#endif // WIFCONTINUED
    log_crit("Unknown process status");
    return -2;
}

int kill_with_log(int sig) {
    int res = kill(-1, sig);
    if (res < 0 && errno != ESRCH) {
        log_crit_e("Error sending signal to children", errno);
        return -1;
    } else if (res < 0) {
        log_info_f("No children to send signal (%s) to", strsignal(sig));
        return 0;
    } else {
        log_info_f("Sent signal (%s) to some children", strsignal(sig));
        return 0;
    }
}

int wait_all(bool nonblock) {
    while (true) {
        int status;
        pid_t pid = waitpid(-1, &status, nonblock ? WNOHANG : 0);
        if (pid < 0) {
            log_crit_e("Error waiting for terminated processes", errno);
            return -1;
        }
        if (pid == 0) {
            return 0;
        }

        if (log_status(pid, status) < 0) {
            log_crit("Error writing child status");
            return -1;
        }
    }
}

void free_tasks(InitTasks init_tasks) {
    for (int task_i = 0; task_i < init_tasks.task_count; ++task_i) {
        free_task(&init_tasks.tasks[task_i]);
    }
    free(init_tasks.tasks);
}

void free_all() {
    free(config_path);
    free_tasks(tasks);
    free(pids);
}

int terminate_tasks() {
    kill_with_log(SIGTERM);

    log_info_f("Politely sleeping for %d ms to wait for children to terminate...", WAIT_FOR_CHILDREN_TO_TERMINATE_MS);

    struct timespec ts = {
            WAIT_FOR_CHILDREN_TO_TERMINATE_MS / 1000,
            WAIT_FOR_CHILDREN_TO_TERMINATE_MS * 1000000 };
    if (nanosleep(&ts, NULL) < 0) {
        log_crit_e("Couldn't wait for children to terminate", errno);
        return -1;
    }

    if (wait_all(true) < 0) {
        log_crit("Error waiting for processes after SIGTERM");
        return -1;
    }

    kill_with_log(SIGKILL); // is it always possible to kill child?

    log_info("Waiting for all children to terminate...");
    if (wait_all(false) < 0) {
        log_crit("Error waiting for processes after SIGKILL");
        return -1;
    }

    free_tasks(tasks);
    free(pids);

    return 0;
}

int reset(int sig) {
    if (!started_tasks) {
        log_info("Received reset command before ")
    }

    if (terminate_tasks() < 0) {
        log_crit("Couldn't terminate tasks");

        free_all();
        close_log();
        exit(-1);
    }

}


int start_task(int idx) {
    int exec_res = exec_task(&tasks.tasks[idx], &pids[idx]);
    if (exec_res == -2) {
        return -2;
    } else if (exec_res == -1) {

        pids[idx] = -1;
        return -1;
    }
    return 0;
}

int start_tasks() {
    pids = malloc(sizeof(pid_t) * tasks.task_count);
    if (pids == NULL) {
        log_crit_e("Couldn't allocate memory for pids", errno);
        return -1;
    }

    for (int i = 0; i < tasks.task_count; i++) {
        if (start_task(i) == -2) {
            log_crit("Error execing task");
            return -1;
        }
    }
    return 0;
}

int restart_task(pid_t pid) {
    int task_i = -1;
    for (int i = 0; i < tasks.task_count; ++i) {
        if (pids[i] == pid) {
            task_i = i;
            break;
        }
    }
    if (task_i == -1) {
        log_crit("Couldn't find task by pid");
        return -1;
    }


}

// -1 if error, 0 if no tasks alive
int wait_tasks() {
    while (true) {
        int status;
        pid_t pid = waitpid(-1, &status, 0); // dont track stop/continue
        if (pid < 0 && errno == ESRCH) {
            log_info("No tasks to wait; restarting");
            return 0;
        } else if (pid < 0 && errno != EINTR) {
            log_crit_e("Error trying to wait task", errno);
            return -1;
        } else { // terminated (since we aren't tracking stop/continue)


        }
    }
}

int read_config_and_run() {
    started_tasks = false;

    if (parse_config(config_path, &tasks) < 0) {
        log_crit("Error parsing config");

        free(config_path);
        return -1;
    }
    log_info("Successfully parsed config file");

    if (tasks.task_count == 0) {
        log_info("No tasks parsed from config. Waiting for SIGHUP...");
        pause();
    }

    if (start_tasks() < 0) {
        log_crit("Error starting tasks");

        free_tasks(tasks);
        free(config_path);
        return -1;
    }

    started_tasks = true;
    return 0;
}

int run_daemon(int argc, char* argv[]) {
    //log_warning_f1e("My msg %s", "my str", 8);

    log_info("Running myinit");

    config_path = parse_config_path(argc, argv);
    if (config_path == NULL) {
        log_crit("Couldn't get config path");
        return -1;
    }
    log_info("Successfully got config path");

    if (parse_config(config_path, &tasks) < 0) {
        log_crit("Error parsing config");

        free(config_path);
        return -1;
    }
    log_info("Successfully parsed config file");

    if (tasks.task_count == 0) {
        log_info("No tasks parsed from config. Waiting for SIGHUP...");
        pause();
    }

    if (start_tasks() < 0) {
        log_crit("Error starting tasks");

        free_tasks(tasks);
        free(config_path);
        return -1;
    }

    flush_log();
    wait_tasks();

    log_info("Shutting down...");
    return 0;
}