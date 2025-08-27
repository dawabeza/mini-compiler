#ifndef ERROR_HANLING_H
#define ERROR_HANDLING_H
struct  lexer_state;
void report_lexer_error(struct lexer_state *lexer_state, char *error_message);
#endif