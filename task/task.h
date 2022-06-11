#ifndef UNTITLED_TASK_H
#define UNTITLED_TASK_H

#include <sys/types.h>

typedef struct {
    int argc;
    char** argv; // null-terminated of null-terminated

    char* stdin_path;
    char* stdout_path;
} Task;

int create_task(Task* dest, char** argv, int argc);

char* format_task(Task* dest);

int exec_task(const Task* dest, pid_t* task_pid);

void free_task(Task* dest);

#endif //UNTITLED_TASK_H
