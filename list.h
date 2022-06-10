#ifndef UNTITLED_LIST_H
#define UNTITLED_LIST_H

#include <sys/types.h>
#include <malloc.h>
#include <memory.h>

#define INIT_COUNT 1

typedef struct {
    pid_t* buf;

    size_t capacity;
    size_t count;
} PidList;

int GetList(PidList* dest) {
    dest->buf = malloc(sizeof(pid_t) * INIT_COUNT);
    if (dest->buf == NULL) {
        perror("Couldn't allocate list");
        return -1;
    }

    dest->capacity = INIT_COUNT;
    dest->count = 0;
    return 0;
}

size_t GetNewCapacity(size_t old_capacity) {
    return old_capacity * 2;
}

int Add(PidList* list, pid_t pid) {
    if (list->capacity == list->count) {
        size_t new_capacity = GetNewCapacity(list->capacity);

        pid_t* new_buf = malloc(sizeof(pid_t) * new_capacity);
        if (new_buf == NULL) {
            perror("Couldn't allocate new list");
            return -1;
        }

        memcpy(new_buf, list->buf, list->capacity);
        free(list->buf);

        list->buf = new_buf;
        list->capacity = new_capacity;
    }
    list->buf[list->count++] = pid;
    return 0;
}



#endif //UNTITLED_LIST_H
