#include "error_handling.h"
#include "lexer.h"
#include <stdio.h>

void report_error(struct lexer_state *lexer_state, char *error_message, int cur_start, int cur_end)
{
    printf("ERROR at %d:%d %s\n",  lexer_state->line, cur_end - lexer_state->cur_line_start, error_message);
    lexer_state->had_error = 1;
}