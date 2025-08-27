#include "lexer.h"
#include "error_handling.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

char *FULL_LANGUAGE[] = {";", "=", "+=", "-=", "*=", "/=", "+",  "/", "*", "-", ",", "%", "#", "(", ")", "{", 
                    "}", ">", "<", ">=", "<=", "==", "if", "else", "while", "for", "fun", "var", "true", "false"};
enum token_type FULL_LANGUAGE_TYPE[] = {TOKEN_SEMICOLON, TOKEN_ASSIGNMENT, TOKEN_PLUS_ASSIGN, TOKEN_MINUS_ASSIGN, TOKEN_STAR_ASSIGN,
                                       TOKEN_SLASH_ASSIGN, TOKEN_PLUS, TOKEN_SLASH, TOKEN_STAR, TOKEN_MINUS, TOKEN_COMMA, TOKEN_MODULO, 
                                       TOKEN_BANG, TOKEN_OPEN_PARENTHESIS, TOKEN_CLOSED_PARENTHESIS, TOKEN_OPEN_CURLY, TOKEN_CLOSED_CURLY,
                                       TOKEN_GREATER_THAN, TOKEN_GREATER_EQUAL, TOKEN_LESS_THAN, TOKEN_LESS_EQUAL, TOKEN_EQUAL,
                                       TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_FOR, TOKEN_FUN, TOKEN_VAR, TOKEN_TRUE, TOKEN_FALSE
                                       };


void init_lexer_state(struct lexer_state *lexer_state)
{
    lexer_state->had_error = 0;
    lexer_state->line = lexer_state->cur_line_start = 1;
    lexer_state->cur_char_p = lexer_state->cur_token_begining = 0;
    init_token_list(&lexer_state->token_list);
    init_mystring(&lexer_state->input_str);
    get_stream(&lexer_state->input_str);
    init_op_keyword(lexer_state);
}

void destroy_lexer_state(struct lexer_state *lexer_state)
{
    for (int i = 0; i < lexer_state->token_list.count; i++) {
        struct token *token = lexer_state->token_list.data[i]; 
        free(token->lexeme);
        free(token->literal);
        free(token);
    }
    free(lexer_state->input_str.data);
    free(lexer_state->token_list.data);
}

void scan(struct lexer_state *lexer_state)
{
    while (!is_at_end(lexer_state)) {
        char cur_char = peek_char(lexer_state);
        if (isspace(cur_char) && cur_char != '\n') {
            advance_char(lexer_state);
        }
        else if (isdigit(cur_char)) {
            scan_num(lexer_state);
        }
        else if (isalpha(cur_char) || cur_char == '_') {
            scan_name(lexer_state);
        }
        else if (cur_char == '\"') {
            scan_str_literal(lexer_state);
        }
        else if (find_operator(lexer_state, cur_char) != -1) {
            scan_operator(lexer_state);
        }
        else if (cur_char == '#') {
            skip_comment(lexer_state);
        }
        else if (cur_char == '\n') {
            lexer_state->line++;
            lexer_state->cur_line_start = lexer_state->cur_char_p;
            advance_char(lexer_state);
        }
        else {
            report_lexer_error(lexer_state, "Invalid charachter");
            skip_invalid(lexer_state);
        }
    }
}

void add_token(struct lexer_state *lexer_state, char *lexeme, enum token_type type, struct literal *literal)
{
    struct token *new_token = malloc(sizeof(struct token));
    if (!new_token) {
        printf("MEMORY ALLOCATION FAILED");
        exit(1);
    }
    init_token(new_token, lexeme, type, lexer_state->cur_token_begining, lexer_state->cur_char_p, literal, lexer_state->line);
    push_back(&lexer_state->token_list, new_token);
}

void scan_num(struct lexer_state *lexer_state)
{
    save_token_begining(lexer_state);
    //the digit part
    long double num = 0;
    while (isdigit(peek_char(lexer_state))) {
        num = num * 10 + peek_char(lexer_state) - '0';
        advance_char(lexer_state);
    }
    //fractional part
    long double frac_part = 0;
    int frac_power = 1;
    if (peek_char(lexer_state) == '.') {
        advance_char(lexer_state);
        while (isdigit(peek_char(lexer_state))) {
            frac_part = frac_part * 10 + peek_char(lexer_state) - '0';
            frac_power *= 10;
            advance_char(lexer_state);
        }
    }
    //exponent e/E
    int exp = 0;
    if (peek_char(lexer_state) == 'e' || peek_char(lexer_state) == 'E') {
        advance_char(lexer_state);
        while (isdigit(peek_char(lexer_state))) {
            exp = exp * 10 + peek_char(lexer_state) - '0';
            advance_char(lexer_state);
        }
    }
    //get the value of the full number
    num += frac_part / frac_power;
    num *= pow(10, exp);
    //handle trailing non_digit charachters if any
    if (isalnum(peek_char(lexer_state)) || peek_char(lexer_state) == '.') {
        report_lexer_error(lexer_state, "INVALID NUMBER");
        skip_invalid(lexer_state);
    }
    else {
        char *lexeme = get_cur_token_lexeme(lexer_state);
        if (!lexeme) {
            printf("FAILED MEMORY ALLOCATION");
            exit(1);
        }
        //this literal is only valid for string literal and number literal token types
        struct literal *num_literal = make_literal(TOKEN_NUMBER, num, NULL);
        add_token(lexer_state, lexeme, TOKEN_NUMBER, num_literal);
    }
}

void scan_name(struct lexer_state *lexer_state)
{
    save_token_begining(lexer_state);
    while (isalnum(peek_char(lexer_state)) || peek_char(lexer_state) == '_') {
        advance_char(lexer_state);
    }
    char *lexeme = get_cur_token_lexeme(lexer_state);
    if (find_keyword(lexer_state, lexeme) != -1) {
        add_token(lexer_state, lexeme, get_token_type(lexer_state, lexeme), NULL);
    }
    else {
        add_token(lexer_state, lexeme, TOKEN_VAR, NULL);
    }
}

void scan_str_literal(struct lexer_state *lexer_state)
{
    save_token_begining(lexer_state);
    advance_char(lexer_state);
    while (peek_char(lexer_state) != '\"' && peek_char(lexer_state) != '\n' && peek_char(lexer_state) != '\0')
        advance_char(lexer_state);

    if (peek_char(lexer_state) == '\n' || peek_char(lexer_state) == '\0') {
        report_lexer_error(lexer_state, "No closing double quote for string");
    }
    else {
        advance_char(lexer_state);
        char *lexeme = get_cur_token_lexeme(lexer_state);
        struct literal *str_literal = make_literal(TOKEN_STR_LITERAL, 0, lexeme);
        add_token(lexer_state, lexeme, get_token_type(lexer_state, lexeme), str_literal); 
    }
}

void scan_operator(struct lexer_state *lexer_state)
{
    save_token_begining(lexer_state);
    advance_char(lexer_state);
    switch(prev_char(lexer_state)) {
        case '+': case '-': case '*': case '/': case '>': case '<': case '=':
            if (peek_char(lexer_state)  == '=') {
                advance_char(lexer_state);
            }
            break;
    }
    char *lexeme = get_cur_token_lexeme(lexer_state);
    add_token(lexer_state, lexeme, get_token_type(lexer_state, lexeme), NULL);
}

void skip_comment(struct lexer_state *lexer_state)
{
    while (peek_char(lexer_state) != '\n' && peek_char(lexer_state) != '\0') 
        advance_char(lexer_state);
}

void get_stream(struct my_string* my_str)
{
    int c;
    while ((c = getchar()) != EOF) {
        str_push_back(my_str, c);
    }
    str_push_back(my_str, '\0');
}

void print_tokens(struct lexer_state *lexer_state)
{
    for (int i = 0; i < lexer_state->token_list.count; i++) {
        printf("%s\n", lexer_state->token_list.data[i]->lexeme);
    }
}

int find_operator(struct lexer_state *lexer_state, char op)
{
    char temp_op[2] = {op, '\0'};
    for (int i = 0; i < OP_KEY_COUNT; i++) {
        if (strcmp(lexer_state->op_name_map[i].name, temp_op) == 0) return i;
    }
    return -1;
}

int find_keyword(struct lexer_state *lexer_state, char *keyword)
{
    for (int i = 0; i < OP_KEY_COUNT; i++) {
        if (strcmp(lexer_state->op_name_map[i].name, keyword) == 0) return i;
    }
    return -1;
}

void skip_invalid(struct lexer_state *lexer_state)
{
     while ( !isspace(peek_char(lexer_state)) && 
             peek_char(lexer_state) != '\n' && 
             peek_char(lexer_state) != '\0' ) 
                advance_char(lexer_state);
}

enum token_type get_token_type(struct lexer_state *lexer_state, char *lexeme)
{
    for (int i = 0; i < OP_KEY_COUNT; i++) {
        if (strcmp(lexer_state->op_name_map[i].name, lexeme) == 0) {
            return lexer_state->op_name_map[i].type;
        }
    }
    return TOKEN_NONE_TOKEN;
}

void init_op_keyword(struct lexer_state *lexer_state)
{
    for (int i = 0; i < OP_KEY_COUNT; i++) {
        lexer_state->op_name_map[i].name = FULL_LANGUAGE[i];
        lexer_state->op_name_map[i].type = FULL_LANGUAGE_TYPE[i];
    }
}

struct literal * make_literal(enum token_type literal_type, int num_val, char * str_val)
{  
    struct literal *new_literal = malloc(sizeof (struct literal));
    if (literal_type == TOKEN_NUMBER) {
        new_literal->literal_type = TOKEN_NUMBER;
        new_literal->literal_val.num_literal = num_val;
    }
    else if (literal_type == TOKEN_STR_LITERAL) {
        new_literal->literal_type = TOKEN_STR_LITERAL;
        new_literal->literal_val.str_literal = str_val;
    }
    else {
        free(new_literal);
        new_literal = NULL;
        return NULL;
    }
    return new_literal;
}

char peek_char(struct lexer_state* lexer_state)
{
    return lexer_state->input_str.data[lexer_state->cur_char_p];
}

char advance_char(struct lexer_state *lexer_state)
{
    if (is_at_end(lexer_state)) {
        return EOF;
    }
    return lexer_state->input_str.data[lexer_state->cur_char_p++];

}

int is_at_end(struct lexer_state *lexer_state)
{
    return peek_char(lexer_state) == '\0';
}

char prev_char(struct lexer_state *lexer_state)
{
    if (lexer_state->cur_char_p == 0) {
        return EOF;
    }

    return lexer_state->input_str.data[lexer_state->cur_char_p - 1];
}

char *get_cur_token_lexeme(struct lexer_state *lexer_state)
{
    struct my_string str;
    init_mystring(&str);
    for (int i =  lexer_state->cur_token_begining; i < lexer_state->cur_char_p; i++) {
        str_push_back(&str, get_char_at(lexer_state, i));
    }
    
    return  str.data;
}

char get_char_at(struct lexer_state *lexer_state, int pos)
{

    return lexer_state->input_str.data[pos];
}

void save_token_begining(struct lexer_state *lexer_state)
{
    lexer_state->cur_token_begining = lexer_state->cur_char_p;
}

int cur_token_begining(struct lexer_state *lexer_state)
{
    return lexer_state->cur_token_begining;
    
}

char *sub_str(struct lexer_state *lexer_state, int low, int high)
{
    int n = high - low;
    struct my_string  my_str;
    init_mystring(&my_str);
    int i;
    for (i = 0; i < n; i++) {
        str_push_back(&my_str, lexer_state->input_str.data[low + i]);
    }
    str_push_back(&my_str, '\0');
    return my_str.data;
}
