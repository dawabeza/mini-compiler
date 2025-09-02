#include "error_handling.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

void report_lexer_error(struct lexer_state *lexer_state, char *error_message)
{
    printf("ERROR at %d:%d %s(LEXER ERROR)\n",  lexer_state->line, lexer_state->cur_char_p - lexer_state->cur_line_start, error_message);
    lexer_state->had_error = 1;
}

void report_parse_error(struct parser_state *parser_state, char *error_message)
{
    printf("ERROR at %d:%d %s(PARSE ERROR)\n", peek_token(parser_state)->line, peek_token(parser_state)->start_pos, error_message);
    parser_state->had_error = 1;
}
