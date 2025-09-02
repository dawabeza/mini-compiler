#ifndef TOKEN_H
#define TOKEN_H
#define MAX_LEXEME 100

enum token_type {
    //operators
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MODULO, TOKEN_OPEN_PARENTHESIS, TOKEN_CLOSED_PARENTHESIS,
    TOKEN_OPEN_CURLY, TOKEN_CLOSED_CURLY, TOKEN_COMMA, TOKEN_EQUAL, TOKEN_SEMICOLON, TOKEN_GREATER_THAN, TOKEN_LESS_THAN,
    TOKEN_GREATER_EQUAL, TOKEN_LESS_EQUAL, TOKEN_ASSIGNMENT, TOKEN_PLUS_ASSIGN, TOKEN_MINUS_ASSIGN, TOKEN_STAR_ASSIGN,
    TOKEN_SLASH_ASSIGN, TOKEN_BANG, TOKEN_BANG_EQUAL, TOKEN_HASH_TAG,
    //keywords
    TOKEN_IF, TOKEN_ELSE, TOKEN_FOR, TOKEN_WHILE, TOKEN_FUN, TOKEN_VAR, TOKEN_TRUE, TOKEN_FALSE,
    //literal
    TOKEN_NUMBER, TOKEN_STR_LITERAL, TOKEN_IDENTIFIER,
    //indicate token type for non-token tokens
    TOKEN_NONE_TOKEN, TOKEN_EOF
};

struct literal {
    enum token_type literal_type;
    union {
        long double num_literal;
        char *str_literal;
    } literal_val;
};

struct token {
    char *lexeme;
    enum token_type token_type;
    struct literal *literal;
    int start_pos;
    int end_pos;
    int line;
};

struct token_list {
    struct token** data;
    int count;
    int capacity;
};

void init_token(struct token *token, char *lexeme, enum token_type token_type, int start_pos, int end_pos, struct literal *literal, int line);
//token list operations
void init_token_list(struct token_list *token_list);
void push_back_token(struct token_list* token_list, struct token* new_token);
#endif