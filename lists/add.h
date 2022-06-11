//
// Created by sergei on 11.06.22.
//

#ifndef UNTITLED_ADD_H
#define UNTITLED_ADD_H

#include <stddef.h>
#include "../task/task.h"

int char_add(char** dest, size_t* count, size_t* capacity, char c);
int string_add(char*** dest, size_t* count, size_t* capacity, char* s);
void free_strings(char** arr, size_t count);
int task_add(Task** dest, size_t* count, size_t* capacity, Task task);


#endif //UNTITLED_ADD_H
