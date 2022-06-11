#include <sys/types.h>
#include <malloc.h>
#include <memory.h>
#include <errno.h>
#include "char_list.h"
#include "../log/log.h"

#define INIT_COUNT 1

int GetCharList(CharList* dest) {
    dest->buf = malloc(sizeof(char) * INIT_COUNT);
    if (dest->buf == NULL) {
        log_crit_e("Couldn't allocate list buffer", errno);
        return -1;
    }

    dest->capacity = INIT_COUNT;
    dest->count = 0;
    return 0;
}

size_t _GetNewCapacity(size_t old_capacity) {
    return old_capacity * 2;
}

int AddChar(CharList* list, char c) {
    if (list->capacity == list->count) {
        size_t new_capacity = _GetNewCapacity(list->capacity);

        char* new_buf = realloc(list->buf, sizeof(char) * new_capacity);
        if (new_buf == NULL) {
            log_crit_e("Couldn't reallocate list buffer", errno);
            return -1;
        }

        memcpy(new_buf, list->buf, list->capacity);
        free(list->buf);

        list->buf = new_buf;
        list->capacity = new_capacity;
    }
    list->buf[list->count++] = c;
    return 0;
}