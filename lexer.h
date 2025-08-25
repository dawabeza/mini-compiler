#ifndef LEXER_H
#define LEXER_H
#define MAX_TOKEN 10000
#define MAX_CHARS 100000
#define OP_KEY_COUNT 30
#include "token.h"
struct token;
enum token_type;
struct literal;
struct name_type_map {
    char *name;
    enum token_type type;
};
struct lexer_state {
    struct token *tokens[MAX_TOKEN];
    int token_p;
    char input_str[MAX_CHARS];
    struct name_type_map name_map[OP_KEY_COUNT];
    int line;
    int had_error;
    int cur_line_start;
};

void init_lexer_state(struct lexer_state *lexer_state);
void destroy_lexer_state(struct lexer_state *lexer_state);

void scan(struct lexer_state *lexer_state);

void add_token(struct lexer_state *lexer_state, char *lexeme, enum token_type type, int start_pos, int end_pos, struct literal literal);
int scan_num(int cur, struct lexer_state *lexer_state);
int scan_name(int cur, struct lexer_state *lexer_state);
int scan_str_literal(int cur, struct lexer_state *lexer_state);
int scan_operator(int cur, struct lexer_state *lexer_state);
int skip_comment(int cur, struct lexer_state *lexer_state);


int find_operator(struct lexer_state *lexer_state, char op);
int find_keyword(struct lexer_state *lexer_state, char *keyword);
int next_valid_start(struct lexer_state *lexer_state, int cur);
enum token_type get_token_type(struct lexer_state *lexer_state, char *lexeme);
void init_op_keyword(struct lexer_state *lexer_state);

void get_stream(char *input_str);
void print_tokens(struct lexer_state *lexer_state);

//string utility functions
char *my_strdup(const char *s);
char *sub_str(struct lexer_state *lexer_state, int low, int high);
#endif