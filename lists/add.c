//
// Created by sergei on 11.06.22.
//

#include "add.h"

#include <stddef.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include "../task/task.h"
#include "../log/log.h"

// yeah, it's list; not a struct bc no generics and editing a lot of funcs in lots of files is meh

size_t _get_new_capacity(size_t old_capacity) {
    return old_capacity + old_capacity / 2;
}

int char_add(char** dest, size_t* count, size_t* capacity, char c) {
    if (*count == *capacity) {

        size_t new_capacity = _get_new_capacity(*capacity);
        char* old_dest = *dest;
        *dest = realloc(*dest, sizeof(char) * new_capacity);
        if (*dest == NULL) {
            log_err_e("Couldn't reallocate buffer memory", errno);
            free(old_dest);
            return -1;
        }
        *capacity = new_capacity;
    }

    (*dest)[*count] = c;
    (*count)++;
    return 0;
}

int string_add(char*** dest, size_t* count, size_t* capacity, char* s) {
    if (*count == *capacity) {

        size_t new_capacity = _get_new_capacity(*capacity);
        char** old_dest = *dest;
        *dest = realloc(*dest, sizeof(char*) * new_capacity);
        if (*dest == NULL) {
            log_err_e("Couldn't reallocate buffer memory", errno);
            free_strings(old_dest, *count);
            return -1;
        }
        *capacity = new_capacity;
    }

    (*dest)[*count] = malloc(sizeof(char) * (strlen(s) + 1));
    if ((*dest)[*count] == NULL) {
        log_err_e("Couldn't allocate memory for string copy", errno);
        return -1;
    }

    strcpy((*dest)[*count], s);
    (*count)++;
    return 0;
}

void free_strings(char** arr, size_t count) {
    for (int i = 0; i < count; i++) {
        free(arr[count]);
    }
    free(arr);
}

int task_add(Task** dest, size_t* count, size_t* capacity, Task task) {
    if (*count == *capacity) {

        size_t new_capacity = _get_new_capacity(*capacity);
        Task* old_dest = *dest;
        *dest = realloc(*dest, sizeof(Task) * new_capacity);
        if (*dest == NULL) {
            log_err_e("Couldn't reallocate buffer memory", errno);
            free(old_dest);
            return -1;
        }
        *capacity = new_capacity;
    }

    (*dest)[*count] = task;
    (*count)++;
    return 0;
}

int pid_add(pid_t** dest, size_t* count, size_t* capacity, pid_t pid) {
    if (*count == *capacity) {

        size_t new_capacity = _get_new_capacity(*capacity);
        pid_t* old_dest = *dest;
        *dest = realloc(*dest, sizeof(pid_t) * new_capacity);
        if (*dest == NULL) {
            log_err_e("Couldn't reallocate buffer memory", errno);
            free(old_dest);
            return -1;
        }
        *capacity = new_capacity;
    }

    (*dest)[*count] = pid;
    (*count)++;
    return 0;
}
