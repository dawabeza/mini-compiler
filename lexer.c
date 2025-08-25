#include "lexer.h"
#include "token.h"
#include "error_handling.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

char ops[] = {'+', '/', '*', '-', ',', '%', '(', ')', '{', '}'};
char *key_words[] = {"if", "else", "while", "for", "fun", "var"};


void init_lexer_state(struct lexer_state *lexer_state)
{
    lexer_state->had_error = 0;
    lexer_state->line = 1;
    lexer_state->token_p = 0;
    get_stream(lexer_state->input_str);
}

void destroy_lexer_state(struct lexer_state *lexer_state)
{
    for (int i = 0; i < lexer_state->token_p; i++) {
        free(lexer_state->tokens[i]);
    }
    free(lexer_state);
}

void scan(struct lexer_state *lexer_state)
{
    int cur = 0;
    while (lexer_state->input_str[cur] != '\0') {
        char cur_char = lexer_state->input_str[cur];
        if (isdigit(cur_char)) {
            cur = scan_num(cur, lexer_state);
        }
        else if (isalpha(cur_char) || cur_char == '-') {
            cur = scan_name(cur, lexer_state);
        }
        else if (cur_char == '\"') {
            cur = scan_str_literal(cur, lexer_state);
        }
        else if (find_operator(ops, cur_char) != -1) {
            cur = scan_operator(cur, lexer_state);
        }
        else {
            report_error("Invalid charachter", lexer_state->line, -1, cur);
            cur++;
        }
    }
}

void add_token(struct lexer_state *lexer_state, char *lexeme, enum token_type type, int start_pos, int end_pos)
{
    struct token *new_token = malloc(sizeof(struct token));
    if (!new_token) {
        printf("MEMORY ALLOCATION FAILED");
        exit(1);
    }
    init_token(new_token, lexeme, type, start_pos, end_pos, lexer_state->line);
    lexer_state->tokens[lexer_state->token_p++] = new_token;
}

int scan_num(int cur, struct lexer_state *lexer_state)
{
    char temp_lexeme[MAX_LEXEME];
    int i = 0;
    int prev = cur;
    //the digit part
    int num = 0;
    while (isdigit(lexer_state->input_str[cur])) {
        num = num * 10 + lexer_state->input_str[cur] - '0';
        temp_lexeme[i++] = lexer_state->input_str[cur];
        cur++;
    }

    //fractional part
    int frac_part = 0;
    int frac_power = 1;
    if (lexer_state->input_str[cur] == '.') {
        temp_lexeme[i++] = lexer_state->input_str[cur];
        cur++;
        while (isdigit(lexer_state->input_str[cur])) {
            frac_part = frac_part * 10 + lexer_state->input_str[cur] - '0';
            frac_power *= 10;
            temp_lexeme[i++] = lexer_state->input_str[cur];
            cur++;
        }
    }
    //exponent e/E
    int exp = 0;
    if (lexer_state->input_str[cur] == 'e' || lexer_state->input_str[cur] == 'E') {
        temp_lexeme[i++] = lexer_state->input_str[cur];
        cur++;
        while (isdigit(lexer_state->input_str[cur])) {
            exp = exp * 10 + lexer_state->input_str[cur] - '0';
            temp_lexeme[i++] = lexer_state->input_str[cur];
            cur++;
        }
    }

    temp_lexeme[i] = '\0';

    if (!isspace(lexer_state->input_str[cur]) || lexer_state->input_str[cur]  != '\n') {
        report_error("INVALID NUMBER", lexer_state->line, prev, cur);
        while (!isspace(lexer_state->input_str[cur]) && lexer_state->input_str[cur]  != '\n') {
            cur++;
        }
    }
    else {
        char *dup_str = my_strdup(temp_lexeme);
        if (!dup_str) {
            printf("FAILED MEMORY ALLOCATION");
            exit(1);
        }
        add_token(lexer_state, dup_str, TOKEN_NUMBER, prev, cur - 1);
    }

    return cur;
}

int scan_name(int cur, struct lexer_state *lexer_state)
{
    return 0;
}

int scan_str_literal(int cur, struct lexer_state *lexer_state)
{
    return 0;
}

int scan_operator(int cur, struct lexer_state *lexer_state)
{
    return 0;
}

void get_stream(char *input_str)
{
    int i = 0;
    int c;
    while (i < MAX_CHARS && (c = getchar()) != EOF) {
        input_str[i++] = c;
    }
    input_str[i] = '\0';
}

void print_tokens(struct lexer_state *lexer_state)
{
    for (int i = 0; i < lexer_state->token_p; i++) {
        printf("%s---", lexer_state->tokens[i]->lexeme);
    }
}

int find_operator(char *ops, char c)
{
    for (int i = 0; i < OP_COUNT; i++) {
        if (ops[i] == c) return i;
    }
    return -1;
}

int find_keyword(char **keywords, char *keyword)
{
    for (int i = 0; i < KEYWORD_COUNT; i++) {
        if (strcmp(keywords[i], keyword) == 0) return i;
    }
    return -1;
}

char *my_strdup(const char *s) {
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    strcpy(copy, s);
    return copy;
}
