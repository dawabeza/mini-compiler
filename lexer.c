#include "lexer.h"
#include "error_handling.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>



void init_lexer_state(struct lexer_state *lexer_state)
{
    lexer_state->had_error = 0;
    lexer_state->line = 1;
    lexer_state->token_p = 0;
    get_stream(lexer_state->input_str);
    init_name_map(lexer_state);
}

void destroy_lexer_state(struct lexer_state *lexer_state)
{
    for (int i = 0; i < lexer_state->token_p; i++) {
        free(lexer_state->tokens[i]->lexeme);
        free(lexer_state->tokens[i]);
    }
    for (int i = 0; i < OP_KEY_COUNT; i++) {
        free(lexer_state->name_map[i].name);
    }
    free(lexer_state);
}

void scan(struct lexer_state *lexer_state)
{
    int cur = 0;
    while (lexer_state->input_str[cur] != '\0') {
        char cur_char = lexer_state->input_str[cur];
        if (isspace(cur_char)) {
            cur++;
        }
        else if (isdigit(cur_char)) {
            cur = scan_num(cur, lexer_state);
        }
        else if (isalpha(cur_char) || cur_char == '_') {
            cur = scan_name(cur, lexer_state);
        }
        else if (cur_char == '\"') {
            cur = scan_str_literal(cur, lexer_state);
        }
        else if (find_operator(lexer_state, cur_char) != -1) {
            cur = scan_operator(cur, lexer_state);
        }
        else if (cur_char == '\n') {
            lexer_state->line++;
            cur++;
        }
        else {
            report_error(lexer_state, "Invalid charachter", cur, cur);
            cur = next_valid_start(lexer_state, cur);
        }
    }
}

void add_token(struct lexer_state *lexer_state, char *lexeme, enum token_type type, int start_pos, int end_pos, struct literal literal)
{
    struct token *new_token = malloc(sizeof(struct token));
    if (!new_token) {
        printf("MEMORY ALLOCATION FAILED");
        exit(1);
    }
    init_token(new_token, lexeme, type, start_pos, end_pos, literal, lexer_state->line);
    lexer_state->tokens[lexer_state->token_p++] = new_token;
}

int scan_num(int cur, struct lexer_state *lexer_state)
{
    int prev = cur;
    //the digit part
    long double num = 0;
    while (isdigit(lexer_state->input_str[cur])) {
        num = num * 10 + lexer_state->input_str[cur] - '0';
        cur++;
    }
    //fractional part
    long double frac_part = 0;
    int frac_power = 1;
    if (lexer_state->input_str[cur] == '.') {
        cur++;
        while (isdigit(lexer_state->input_str[cur])) {
            frac_part = frac_part * 10 + lexer_state->input_str[cur] - '0';
            frac_power *= 10;
            cur++;
        }
    }
    //exponent e/E
    int exp = 0;
    if (lexer_state->input_str[cur] == 'e' || lexer_state->input_str[cur] == 'E') {
        cur++;
        while (isdigit(lexer_state->input_str[cur])) {
            exp = exp * 10 + lexer_state->input_str[cur] - '0';
            cur++;
        }
    }
    //get the value of the full number
    num += frac_part / frac_power;
    num *= pow(10, exp);
    //handle trailing non_digit charachters if any
    if (isalnum(lexer_state->input_str[cur]) || lexer_state->input_str[cur] == '.') {
        report_error(lexer_state, "INVALID NUMBER", prev, cur);
        cur = next_valid_start(lexer_state, cur);
    }
    else {
        char *lexeme = sub_str(lexer_state, prev, cur);
        if (!lexeme) {
            printf("FAILED MEMORY ALLOCATION");
            exit(1);
        }
        //this literal is only valid for string literal and number literal token types
        struct literal num_literal = {
            .literal_val.num_literal = num,
            .literal_type = TOKEN_NUMBER
        };
        add_token(lexer_state, lexeme, TOKEN_NUMBER, prev, cur - 1, num_literal);
    }

    return cur;
}

int scan_name(int cur, struct lexer_state *lexer_state)
{
    int prev = cur;
    int i = 0;
    while (isalnum(lexer_state->input_str[cur]) || lexer_state->input_str[cur] == '_') {
        cur++;
    }
    char *lexeme = sub_str(lexer_state, prev, cur);
    if (find_keyword(lexer_state, lexeme) != -1) {
        struct literal no_literal =  {
            .literal_type = TOKEN_NONE_TOKEN
        };
        add_token(lexer_state, lexeme, get_token_type(lexer_state, lexeme), prev, cur - 1, no_literal);
    }
    else {
        struct literal no_literal = {
            .literal_type = TOKEN_NONE_TOKEN
        };
        add_token(lexer_state, lexeme, TOKEN_VAR, prev, cur - 1, no_literal);
    }
    return cur;
}

int scan_str_literal(int cur, struct lexer_state *lexer_state)
{
    int prev = cur;
    cur++;
    while (lexer_state->input_str[cur] != '\"' && lexer_state->input_str[cur] != '\n' && lexer_state->input_str[cur] != '\0')
        cur++;
    if (lexer_state->input_str[cur] == '\n' || lexer_state->input_str[cur] == '\0') {
        report_error(lexer_state, "No closing double quote for string", prev, cur);
    }
    else {
        char *lexeme = sub_str(lexer_state, prev, cur + 1);
        cur++;
    }
    
    return cur;
}

int scan_operator(int cur, struct lexer_state *lexer_state)
{
    int prev = cur;
    cur++;
    char *lexeme = sub_str(lexer_state, prev, cur);
    struct literal no_literal = {
        .literal_type = TOKEN_NONE_TOKEN
    };
    add_token(lexer_state, lexeme, get_token_type(lexer_state, lexeme), prev, cur - 1, no_literal);
    return cur;
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
        printf("%s | ", lexer_state->tokens[i]->lexeme);
    }
}

int find_operator(struct lexer_state *lexer_state, char op)
{
    char temp_op[2] = {op, '\0'};
    for (int i = 0; i < OP_KEY_COUNT; i++) {
        if (strcmp(lexer_state->name_map->name, temp_op)) return i;
    }
    return -1;
}

int find_keyword(struct lexer_state *lexer_state, char *keyword)
{
    for (int i = 0; i < OP_KEY_COUNT; i++) {
        if (strcmp(lexer_state->name_map[i].name, keyword) == 0) return i;
    }
    return -1;
}

int next_valid_start(struct lexer_state *lexer_state, int cur)
{
     while ( !isspace(lexer_state->input_str[cur]) && 
             lexer_state->input_str[cur] != '\n' && 
             lexer_state->input_str[cur] != '\0' ) 
                cur++;
    return cur;
}

enum token_type get_token_type(struct lexer_state *lexer_state, char *lexeme)
{
    for (int i = 0; i < OP_KEY_COUNT; i++) {
        if (strcmp(lexer_state->name_map[i].name, lexeme) == 0) {
            return lexer_state->name_map[i].type;
        }
    }
    return TOKEN_NONE_TOKEN;
}

void init_name_map(struct lexer_state *lexer_state)
{
     char *ops[] = {"+", "/", "*", "-", ",", "%", "(", ")", "{", "}", "if", "else", "while", "for", "fun", "var"};
    enum token_type types[] = {TOKEN_PLUS, TOKEN_SLASH, TOKEN_STAR, TOKEN_MINUS, TOKEN_COMMA, TOKEN_MODULO, 
                               TOKEN_OPEN_PARENTHESIS, TOKEN_CLOSED_PARENTHESIS, TOKEN_OPEN_CURLY, TOKEN_CLOSED_CURLY,
                                TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_FOR, TOKEN_FUN, TOKEN_VAR
                              };
    for (int i = 0; i < OP_KEY_COUNT; i++) {
        lexer_state->name_map[i].name = my_strdup(ops[i]);
        lexer_state->name_map[i].type = types[i];
    }
}

char *my_strdup(const char *s) {
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    strcpy(copy, s);
    return copy;
}

char *sub_str(struct lexer_state *lexer_state, int low, int high)
{
    int n = high - low;
    char temp[n + 1];
    int i;
    for (i = 0; i < n; i++) {
        temp[i] = lexer_state->input_str[low + i];
    }
    temp[i] = '\0';
    return my_strdup(temp);
}
