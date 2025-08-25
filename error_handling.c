#include "error_handling.h"
#include <stdio.h>

void report_error(char * error_message, int line, int cur_start, int cur_end)
{
    printf("ERROR at %d: %s\n",  line, error_message);
}