#include "token.h"
#include <string.h>
void init_token(struct token *token, char *lexeme, enum token_type token_type, int start_pos, int end_pos, struct literal literal, int line)
{
    token->lexeme = lexeme;
    token->token_type = token_type;
    token->start_pos = start_pos;
    token->end_pos = end_pos;
    token->line = line;
    if (literal.literal_type == TOKEN_NUMBER) {
        //here we use a struct wrapper for union
        token->literal.literal_val.num_literal = literal.literal_val.num_literal;
    }
    else {
        token->literal.literal_val.str_literal = literal.literal_val.str_literal;
    }
}