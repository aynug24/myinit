#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "task.h"
#include "../log/log.h"

// don't need to free argv, it's all used in task
// -1 if task format error, 0 if ok
int create_task(Task* dest, char** argv, int argc) {
    if (argc < 3) {
        log_err("Task with less than three parameters; need to specify command, stdin and stdout files");
        return -1;
    }
    if (argv[0][0] != '/') {
        log_err("Task's command path is not absolute; need to specify absolute path");
        return -1;
    }

    dest->stdin_path = argv[argc - 2];
    dest->stdout_path = argv[argc - 1];
    dest->argc = argc - 2; // yeaaah...

    dest->argv = argv;
    dest->argv[dest->argc] = NULL;
    return 0;
}

int exec_task(const Task* dest, pid_t* task_pid) {
    *task_pid = fork();
    if (*task_pid < 0) {
        log_crit_e("Couldn't fork for task", errno);
        return -1;
    }
    if (*task_pid == 0) {

        char* cmd = dest->argv[0];
        execv(cmd, dest->argv);

        log_crit_e("Couldn't exec task", errno); // todo
        return -1;
    }
    return 0;
}

void free_task(Task* dest) {
    for (int arg_i = 0; arg_i < dest->argc; arg_i++) {
        free(dest->argv[arg_i]);
    }
    free(dest->argv);

    free(dest->stdin_path);
    free(dest->stdout_path);
}