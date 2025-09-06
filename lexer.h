#ifndef LEXER_H
#define LEXER_H
#define MAX_TOKEN 10000
#define MAX_CHARS 100000
#define OP_KEY_COUNT 60
#include "token.h"
#include "utils.h"


struct token;
enum token_type;
struct literal;
struct op_key_name_map {
    char *name;
    enum token_type type;
};

struct lexer_state {
    struct token_list token_list;
    struct my_string input_str;
    struct op_key_name_map op_name_map[OP_KEY_COUNT];
    int had_error;
    int line;
    int cur_line_start;
    int cur_token_begining;
    int cur_char_p;
};

void init_lexer_state(struct lexer_state *lexer_state);
void destroy_lexer_state(struct lexer_state *lexer_state);

void scan(struct lexer_state *lexer_state);

void add_token(struct lexer_state *lexer_state, char *lexeme, enum token_type type, struct literal *literal);
void scan_num(struct lexer_state *lexer_state);
void scan_name(struct lexer_state *lexer_state);
void scan_str_literal(struct lexer_state *lexer_state);
void scan_operator(struct lexer_state *lexer_state);
void skip_comment(struct lexer_state *lexer_state);


int find_operator(struct lexer_state *lexer_state, char op);
int find_keyword(struct lexer_state *lexer_state, char *keyword);
void skip_invalid(struct lexer_state *lexer_state);
enum token_type get_token_type(struct lexer_state *lexer_state, char *lexeme);
void init_op_keyword(struct lexer_state *lexer_state);
struct literal *make_literal(enum token_type literal_type, int num_val, char *str_val);

char peek_char(struct lexer_state *lexer_state);
char advance_char(struct lexer_state *lexer_state);
int is_at_end(struct lexer_state *lexer_state);
char prev_char(struct lexer_state *lexer_state);
char *get_cur_token_lexeme(struct lexer_state *lexer_state);
char get_char_at(struct lexer_state *lexer_state, int pos);
void save_token_begining(struct lexer_state *lexer_state);


void get_stream(struct my_string* my_str);
void print_tokens(struct lexer_state *lexer_state);

//string utility functions
char *sub_str(struct lexer_state *lexer_state, int low, int high);
#endif