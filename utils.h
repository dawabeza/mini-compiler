#ifndef UTILS_H
#define UTILS_H

struct my_string {
    char *data;
    int count;
    int capacity;
};

void init_mystring(struct my_string* my_string);
void str_push_back(struct my_string *my_string, char  c);
#endif