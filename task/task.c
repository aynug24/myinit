#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include "task.h"
#include "../log/log.h"
#include "../lists/add.h"

#define INIT_FMT_BUF_SIZE 256

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

    if (dest->stdin_path[0] != '/') {
        log_err("Task's stdin path is not absolute; need to specify absolute path");
        return -1;
    }
    if (dest->stdout_path[0] != '/') {
        log_err("Task's stdout path is not absolute; need to specify absolute path");
        return -1;
    }

    dest->argv = argv;
    dest->argv[dest->argc] = NULL;
    return 0;
}

int _string_append(char** dest, size_t* count, size_t* capacity, char* s) {
    for (int i = 0; s[i] != '\0'; ++i) {
        int add_res = char_add(dest, count, capacity, s[i]);
        if (add_res < 0) {
            return add_res;
        }
    }
    return 0;
}

char* format_task(Task* dest) {
    size_t res_capacity = INIT_FMT_BUF_SIZE;
    size_t res_size = 0;
    char* res = malloc(INIT_FMT_BUF_SIZE);
    if (res == NULL) {
        log_crit_e("Couldn't allocate formatted task", errno);
        return NULL;
    }

    bool ok = false;
    for (int arg_i = 0; arg_i < dest->argc; arg_i++) {
        if (_string_append(&res, &res_size, &res_capacity, dest->argv[arg_i]) < 0
            || char_add(&res, &res_size, &res_capacity, ' ') < 0) {

            log_crit("Couldn't append arg string");
            ok = false;
            goto return_if_ok;
        }
    }

    if (char_add(&res, &res_size, &res_capacity, '<') < 0
        || _string_append(&res, &res_size, &res_capacity, dest->stdin_path) < 0
        || char_add(&res, &res_size, &res_capacity, ' ') < 0) {

        log_crit("Couldn't append stdin redirect");
        ok = false;
        goto return_if_ok;
    }

    if (char_add(&res, &res_size, &res_capacity, '>') < 0
             || _string_append(&res, &res_size, &res_capacity, dest->stdout_path) < 0
             || char_add(&res, &res_size, &res_capacity, ' ') < 0) {

        log_crit("Couldn't append stdout redirect");
        ok = false;
        goto return_if_ok;
    }

    return_if_ok:
    return ok ? res : NULL;
}

// -2 if system error, -1 if task stdin/out file error
int exec_task(const Task* dest, pid_t* task_pid) {
    flush_log();

    int stdin_file = open(dest->stdin_path, O_RDONLY);
    if (stdin_file < 0) {
        log_warning_f1e("Couldn't open stdin file (%s) for task", dest->stdin_path, errno);
        return -1;
    }

    int stdout_file = open(dest->stdout_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stdout_file < 0) {
        log_warning_f1e("Couldn't open/create stdout file (%s) for task", dest->stdout_path, errno);
        return -1;
    }

    *task_pid = fork();
    if (*task_pid < 0) {

        log_crit_e("Couldn't fork for task", errno);
        return -2;
    }
    if (*task_pid == 0) {

        if (dup2(stdin_file, STDIN_FILENO) < 0
            || dup2(stdout_file, STDOUT_FILENO) < 0) {
            log_crit_e("Couldn't duplicate fd to stdin or stdout", errno);
            exit(-1);
        }

        close_log();

        char* cmd = dest->argv[0];
        execv(cmd, dest->argv);

        open_log(); // idk how not to give child log fd but log exec failure, and what to do if this open fails
        log_crit_e("Couldn't exec task", errno);
        close_log();

        exit(-1);
    }

    if (close(stdin_file) < 0
        || close(stdout_file) < 0) {
        log_err_e("Couldn't close task fds", errno);
        return -2;
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