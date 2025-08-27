#include "token.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
void init_token(struct token *token, char *lexeme, enum token_type token_type, int start_pos, int end_pos, struct literal *literal, int line)
{
    token->lexeme = lexeme;
    token->token_type = token_type;
    token->start_pos = start_pos;
    token->end_pos = end_pos;
    token->line = line;
    token->literal = literal;
}

void init_token_list(struct token_list *token_list)
{
    token_list->count = 0;
    token_list->capacity = 8;//initial capacity
    token_list->data = (struct token **)malloc(token_list->capacity * sizeof(struct token *));
    if (!token_list->data) {
        printf("MEMORY ALLOCATION FAILED");
        exit(1);
    }
}

void push_back(struct token_list *token_list, struct token *new_token)
{
    if (token_list->count == token_list->capacity) {
        token_list->capacity *= 2;
        token_list->data = realloc(token_list->data, token_list->capacity * sizeof (struct token *));
        if (!token_list->data) {
            printf("MEMORY ALLOCATION FAILED");
            exit(1);
        }
    }
    token_list->data[token_list->count++] = new_token;
}
