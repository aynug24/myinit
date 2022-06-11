#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "parse_config.h"
#include "../log/log.h"
#include "../task/task.h"
#include "../lists/add.h"

#define INIT_LINE_BUF_SIZE 128
#define INIT_ARGS_BUF_SIZE 32
#define INIT_TASKS_BUF_SIZE 32

// memory management is fun (not sure if freed everything)! not having a generic list is fun!


// *line should be NULL
// -1 if error, 0 if eof, 1 if ok
int fgetline(FILE* file, char** line, int* line_length) {
    if (*line != NULL) {
        log_crit("Wrong argument to fgetline, *line isn't NULL");
        return -1;
    }

    int buf_len = INIT_LINE_BUF_SIZE;
    *line = malloc(buf_len);
    if (*line == NULL) {
        log_crit_e("Couldn't allocate config line buffer", errno);
        return -1;
    }

    *line_length = 0;
    int c;
    while (true) {
        c = fgetc(file);
        if (c == EOF) {
            return 0;
        }
        if (c == '\n') {
            return 1;
        }

        if (*line_length == buf_len) {
            buf_len *= 2;
            char* old_line = *line;
            *line = realloc(*line, buf_len);
            if (*line == NULL) {
                log_crit_e("Couldn't reallocate line buffer", errno);
                free(old_line);
                return -1;
            }
        }

        (*line)[*line_length] = (char)c;
        (*line_length)++;
    }
}

// my f god...
int _parse_config_line(char* config_line, char*** argv, size_t* argc) {
    size_t argv_capacity = INIT_ARGS_BUF_SIZE;
    *argv = malloc(argv_capacity);
    if (*argv == NULL) {
        log_crit_e("Couldn't allocate args buffer", errno);
        return -1;
    }

    *argc = 0;
    char* arg = strtok(config_line, " ");
    while (arg != NULL) {

        if (string_add(argv, argc, &argv_capacity, arg) < 0) {
            log_crit("Couldn't add new argument");
            return -1;
        }

        arg = strtok(NULL, " ");
    }

    return 0;
}

// -2 if error, -1 if line format error, 0 if eof, 1 otherwise (ok)
int _process_config_line(FILE* config, InitTasks* dest, size_t* dest_capacity) {
    char* config_line = NULL;
    int line_length;

    int read_res = fgetline(config, &config_line, &line_length);
    if (read_res < 0) {
        log_crit("Error reading config line");
        return -2;
    }
    if (read_res == 0) {
        free(config_line);
        return 0;
    }

    char** argv = NULL;
    size_t argc;
    if (_parse_config_line(config_line, &argv, &argc) < 0) {
        log_crit("Error parsing config line");

        free(config_line);
        return -2;
    }

    if (argc == 0) { // empty line, may be last line in file
        free_strings(argv, argc);
        free(config_line);
        return -1;
    }

    Task new_task;
    if (create_task(&new_task, argv, (int)argc) < 0) {
        log_warning("Couldn't create task from config line");

        free_strings(argv, argc);
        free(config_line);
        return -2;
    }

    if (task_add(&dest->tasks, &dest->task_count, dest_capacity, new_task) < 0) {
        log_err("Error adding new task");
        return -2;
    }

    return 1;
}

int parse_config(const char* config_path, InitTasks* dest) {
    FILE* config = fopen(config_path, "r");
    if (config == NULL) {
        log_crit_e("Couldn't open config file", errno);
        return -1;
    }

    size_t tasks_capacity = INIT_TASKS_BUF_SIZE;
    dest->task_count = 0;
    dest->tasks = malloc(sizeof(Task) * tasks_capacity);
    if (dest->tasks == 0) {
        log_crit_e("Couldn't allocate tasks buffer", errno);
        fclose(config);
        return -1;
    }

    while (true) {
        int line_res = _process_config_line(config, dest, &tasks_capacity);
        if (line_res == -2) {
            log_crit("Error while processing config line");

            free(dest->tasks);
            fclose(config);
            return -1;
        } else if (line_res == -1) {
            log_warning("Line format error");

            continue;
        } else if (line_res == 0) {
            break;
        }
    }

    fclose(config);
    return 0;
}