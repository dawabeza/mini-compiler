#include "error_handling.h"
#include "lexer.h"
#include <stdio.h>

void report_lexer_error(struct lexer_state *lexer_state, char *error_message)
{
    printf("ERROR at %d:%d %s\n",  lexer_state->line, lexer_state->cur_char_p - lexer_state->cur_line_start, error_message);
    lexer_state->had_error = 1;
}