#include "utils.h"
#include <stdlib.h>
#include <stdio.h>

void init_mystring(struct my_string* my_string)
{
    my_string->count = 0;
    my_string->capacity = 8;
    my_string->data = (char *)malloc(my_string->capacity);
    if (!my_string->data) {
        printf("MEMORY ALLOCATION FAILED");
        exit(1);
    }
}

void str_push_back(struct my_string *my_string, char c)
{
    if (my_string->count == my_string->capacity) {
        my_string->capacity *= 2;
        my_string->data = realloc(my_string->data, my_string->capacity);
        if (!my_string->data) {
            printf("MEMORY ALLOCATION FAILED");
            exit(1);
        }
    }

    my_string->data[my_string->count++] = c;
    my_string->data[my_string->count] = '\0';
}
