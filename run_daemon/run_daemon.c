#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include "run_daemon.h"
#include "../parse/parse_config.h"
#include "../parse/parse_myinit_args.h"
#include "../log/log.h"

#define WAIT_FOR_CHILDREN_TO_TERMINATE_MS 500

char* config_path;

InitTasks tasks;
pid_t* pids;

// set to 1 by handler
volatile sig_atomic_t should_restart = 0;


void free_tasks(InitTasks init_tasks) {
    for (int task_i = 0; task_i < init_tasks.task_count; ++task_i) {
        free_task(&init_tasks.tasks[task_i]);
    }
    free(init_tasks.tasks);
}

void exit_on_sig(int sig) { // log might already be closed (if called after closing it for other reason)
    log_info_f("Shutting down on signal %d (%s)", sig, strsignal(sig));

    // free_tasks(tasks); // might free twice => und behavior; better to not free at all i guess
    // free(pids);
    close_log();

    exit(128 + sig);
}

int add_sig_exits() {
    struct sigaction exit_action;
    exit_action.sa_handler = exit_on_sig;
    sigemptyset(&exit_action.sa_mask);
    sigaddset(&exit_action.sa_mask, SIGHUP);
    sigaddset(&exit_action.sa_mask, SIGINT);
    sigaddset(&exit_action.sa_mask, SIGTERM);

    if (sigaction(SIGINT, &exit_action, NULL) < 0) {
        log_crit_e("Couldn't register SIGINT action", errno);
        return -1;
    }
    if (sigaction(SIGTERM, &exit_action, NULL) < 0) {
        log_crit_e("Couldn't register SIGTERM action", errno);
        return -1;
    }
    if (sigaction(SIGSEGV, &exit_action, NULL) < 0) {
        log_crit_e("Couldn't register SIGSEGV action", errno);
        return -1;
    }
    if (sigaction(SIGABRT, &exit_action, NULL) < 0) {
        log_crit_e("Couldn't register SIGABRT action", errno);
        return -1;
    }
    return 0;
}

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

// -1 if err, 0 if no children, 1 if sent to some
int kill_with_log(int sig) {
    int res = kill(-1, sig); // NO EINTR
    if (res < 0 && errno != ESRCH) {
        log_crit_e("Error sending signal to children", errno);
        return -1;
    } else if (res < 0) {
        log_info_f("No children to send signal (%s) to", strsignal(sig));
        return 0;
    } else {
        log_info_f("Sent signal (%s) to some children", strsignal(sig));
        return 1;
    }
}

// -1 if error, 0 if no children, 1 if would block
int wait_all(bool nonblock) {
    while (true) {
        int status;
        pid_t pid = waitpid(-1, &status, nonblock ? WNOHANG : 0);
        if (pid < 0 && errno == ECHILD) {
            log_info("No children to wait for");
            return 0;
        } else if (pid < 0 && errno == EINTR) { // interrupted; go ahead
            continue;
        } else if (pid < 0) {
            log_crit_e("Error waiting for terminated processes", errno);
            return -1;
        } else if (pid == 0) { // WNOHANG & there are processes & would block
            return 1;
        }

        if (log_status(pid, status) < 0) {
            log_crit("Error writing child status while waiting all children");
            return -1;
        }
    }
}


void free_all() {
    free(config_path);
    free_tasks(tasks);
    free(pids);
}

int sleep_no_intr(struct timespec ts) {
    int res;
    do {
        res = nanosleep(&ts, &ts);
    } while (res < 0 && errno == EINTR);

    if (res < 0 && errno != EINTR) {
        log_crit_e("Error sleeping", errno);
        return -1;
    }
    return 0;
}

// doesn't use tasks.tasks or pids
int terminate_tasks() {
    int kill_res = kill_with_log(SIGTERM);
    if (kill_res < 0) {
        log_crit("Error trying to send SIGTERM");
        return -1;
    } else if (kill_res == 0) {
        log_info("No tasks to send SIGTERM to");
        return 0;
    }

    log_info_f("Politely sleeping for %d ms to wait for children to terminate...", WAIT_FOR_CHILDREN_TO_TERMINATE_MS);

    struct timespec ts = {
            WAIT_FOR_CHILDREN_TO_TERMINATE_MS / 1000,
            WAIT_FOR_CHILDREN_TO_TERMINATE_MS * 1000000 };
    if (sleep_no_intr(ts) < 0) {
        log_crit("Couldn't wait for children to terminate");
        return -1;
    }

    if (wait_all(true) < 0) {
        log_crit("Error waiting for processes after SIGTERM");
        return -1;
    }

    kill_res = kill_with_log(SIGKILL); // is it always possible to kill child?
    if (kill_res < 0) {
        log_crit("Error sending SIGKILL to chilren");
        return -1;
    } else if (kill_res == 0) {
        log_info("No children to send SIGKILL to");
        return 0;
    }

    log_info("Waiting for all children to terminate after SIGKILL...");
    if (wait_all(false) < 0) {
        log_crit("Error waiting for processes after SIGKILL");
        return -1;
    }

    return 0;
}

// NO! LOGS! (=> printf-s) IN! SIG!
void set_restart(int sig) {

    should_restart = 1;
}

int add_restart_handler() {
    struct sigaction restart_action;
    restart_action.sa_handler = set_restart;

    if (sigaction(SIGHUP, &restart_action, NULL) < 0) {
        log_crit_e("Couldn't register restart action", errno);
        return -1;
    }
    return 0;
}

int start_task(int idx) {
    char* task_fmt = format_task(&tasks.tasks[idx]);
    if (task_fmt == NULL) {
        log_crit("Couldn't format task before starting it");
        return -2;
    }

    log_info_f("(Re-)Starting task idx=%d '%s'", idx, task_fmt);

    int exec_res = exec_task(&tasks.tasks[idx], &pids[idx]);
    if (exec_res == -2) {
        return -2;
    } else if (exec_res == -1) {
        log_info_f("Couldn't start task idx=%d '%s'", idx, task_fmt);

        pids[idx] = -1;
        return -1;
    } else {
        log_info_f("Started task idx=%d '%s' with pid=%d", idx, task_fmt, pids[idx]);
        return 0;
    }
}

// -1 if err, 0 if should restart, 1 if ok
int start_tasks() {
    pids = malloc(sizeof(pid_t) * tasks.task_count);
    if (pids == NULL) {
        log_crit_e("Couldn't allocate memory for pids", errno);
        return -1;
    }

    for (int i = 0; i < tasks.task_count; i++) {
        if (should_restart) {
            return 0;
        }
        if (start_task(i) == -2) {
            log_crit("Error execing task");
            return -1;
        }
    }
    return 1;
}

int get_started_tasks_count() {
    int res = 0;
    for (int i = 0; i < tasks.task_count; ++i) {
        if (pids[i] != -1) {
            res++;
        }
    }
    return res;
}

int get_task_by_pid(pid_t pid, int* task_idx) {
    for (int i = 0; i < tasks.task_count; ++i) {
        if (pids[i] == pid) {
            *task_idx = i;
            return 0;
        }
    }
    return -1;
}

// same as start_task, -2 if err, -1 if format err, 0 if ok
int restart_task(pid_t pid) {
    int task_idx;
    if (get_task_by_pid(pid, &task_idx) < 0) {
        log_crit("No task with given pid found to restart");
        return -2;
    }

    return start_task(task_idx);
}

// main loop
// -1 if error, 0 if no tasks alive, 1 if need to restart
int wait_tasks() {
    while (true) {
        if (should_restart) {
            return 1;
        }

        /* !!! RACE CONDITION THERE
         signal can be sent after check and waitpid() will still block
         idk how to get it right; might always add some fake task like sleep 0.1
         so that block is not really long (if the issue is critical);
         or might emulate timeout with some magic from stackoverflow  */

        int status;
        pid_t pid = waitpid(-1, &status, 0); // dont track stop/continue
        if (pid < 0 && errno == ECHILD) {

            log_info("No tasks to wait");
            return 0;
        } else if (pid < 0 && errno == EINTR) {

            continue; // interrupted; try again (prob SIGHUP and will return on next try)
        } else if (pid < 0) {

            log_crit_e("Error trying to wait task", errno);
            return -1;
        } else { // found pid & status; don't track stop/cont => always terminated

            log_status(pid, status);

            if (restart_task(pid) < 0) {
                log_info_f("Error restarting task with pid=%d", pid);
            }
        }
    }
}

/* For handling other would-be race condition:
 if no correct tasks available [RACE CONDITION THERE], program pauses until sig
 (incorrect task => failed to execute at least once; might change this if needed, for example,
 task cannot be execed bc no stdin file, then file is created, and myinit launches it successfully
 after some timeout). */
int sleep_until_restart() {
    struct timespec one_sec = { 1, 0 };
    while (true) {
        if (should_restart) {
            return 0;
        }

        if (nanosleep(&one_sec, NULL) < 0 && errno != EINTR) {
            log_crit_e("Error sleeping", errno);
            return -1;
        }
    }
}

int read_config_and_run() {
    flush_log();

    free_tasks(tasks);
    free(pids);

    should_restart = 0;

    if (terminate_tasks() < 0) {
        log_crit("Error ensuring all children are dead before (re-?)loading daemon");
        return -1;
    }

    if (parse_config(config_path, &tasks) < 0) {
        log_crit("Error parsing config");
        return -1;
    }
    log_info("Successfully parsed config file");

    if (tasks.task_count == 0) {
        log_info("No tasks parsed from config. Waiting for new config...");
        flush_log();
        return sleep_until_restart();
    }

    if (should_restart) {
        return 0;
    }

    if (start_tasks() < 0) {
        log_crit("Error starting tasks");
        return -1;
    }
    log_info("Started (or tried to start) all tasks");

    if (get_started_tasks_count() == 0) {
        log_info("Couldn't start any task. Waiting for new config...");
        flush_log();
        return sleep_until_restart();
    }

    if (should_restart) {
        return 0;
    }

    int wait_res = wait_tasks();
    if (wait_res < 0) {
        log_crit("Error in wait loop");
        return -1;
    } else if (wait_res == 0) {
        log_info("All tasks failed to be launched. Waiting for new config...");
        flush_log();
        return sleep_until_restart();
    } else { // restart
        return 0;
    }
}

int run_daemon(int argc, char* argv[]) {
    log_info("Running myinit");

    if (add_sig_exits() < 0) {
        log_crit("Couldn't add exit signal handlers");
        return -1;
    }

    if (add_restart_handler() < 0) {
        log_crit("Couldn't add restart handler");
        return -1;
    }

    config_path = parse_config_path(argc, argv);
    if (config_path == NULL) {
        log_crit("Couldn't get config path");
        return -1;
    }
    log_info("Successfully got config path");

    while (true) { // set_restart signal => next loop iteration
        if (read_config_and_run() < 0) {
            log_crit("Error while running damon; shutting down");
            break;
        }
        flush_log();
    }

    free_all();
    return -1;
}