#include "token.h"
#include <string.h>
void init_token(struct token *token, char *lexeme, enum token_type token_type, int start_pos, int end_pos, int line)
{
    strcpy(token->lexeme, lexeme);
    token->token_type = token_type;
    token->start_pos = start_pos;
    token->end_pos = end_pos;
    token->line = line;
}