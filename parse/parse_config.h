#ifndef MYINIT_PARSE_CONFIG_H
#define MYINIT_PARSE_CONFIG_H

#include "../task/task.h"

typedef struct {
    size_t task_count;
    Task* tasks;
} InitTasks;

int parse_config(const char* config_path, InitTasks* dest);

#endif //MYINIT_PARSE_CONFIG_H
