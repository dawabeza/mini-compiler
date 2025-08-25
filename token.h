#ifndef TOKEN_H
#define TOKEN_H
#define MAX_LEXEME 100

enum token_type {
    //operators
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MODULO, TOKEN_OPEN_PARENTHESIS, TOKEN_CLOSED_PARENTHESIS,
    TOKEN_OPEN_CURLY, TOKEN_CLOSED_CURLY, TOKEN_COMMA,
    //keywords
    TOKEN_IF, TOKEN_ELSE, TOKEN_FOR, TOKEN_WHILE, TOKEN_FUN, TOKEN_VAR,
    //literal
    TOKEN_NUMBER, TOKEN_STR_LITERAL
};

struct literal {
    enum token_type literal_type;
    union {
        long double num_literal;
        char *str_literal;
    } lexeme;
};

struct token {
    char lexeme[MAX_LEXEME];
    enum token_type token_type;
    struct literal literal;
    int start_pos;
    int end_pos;
    int line;
};

void init_token(struct token *token, char *lexeme, enum token_type token_type, int start_pos, int end_pos, int line);
void destroy_token(struct token *token);
#endif