#ifndef MYINIT_CHAR_LIST_H
#define MYINIT_CHAR_LIST_H

#include <sys/types.h>

typedef struct {
    char* buf;

    size_t capacity;
    size_t count;
} CharList;

int GetCharList(CharList* dest);
int AddChar(CharList* list, char c);


#endif //MYINIT_CHAR_LIST_H
