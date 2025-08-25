#ifndef ERROR_HANLING_H
#define ERROR_HANDLING_H
struct  lexer_state;
void report_error(struct lexer_state *lexer_state, char *error_message, int cur_start, int cur_end);
#endif