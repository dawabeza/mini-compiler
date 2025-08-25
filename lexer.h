#ifndef LEXER_H
#define LEXER_H
#define MAX_TOKEN 10000
#define MAX_CHARS 100000
#define OP_COUNT 10
#define KEYWORD_COUNT 6
struct token;
enum token_type;
struct lexer_state {
    struct token *tokens[MAX_TOKEN];
    int token_p;
    char input_str[MAX_CHARS];
    int line;
    int had_error;
};

void init_lexer_state(struct lexer_state *lexer_state);
void destroy_lexer_state(struct lexer_state *lexer_state);

void scan(struct lexer_state *lexer_state);

void add_token(struct lexer_state *lexer_state, char *lexeme, enum token_type type, int start_pos, int end_pos);
int scan_num(int cur, struct lexer_state *lexer_state);
int scan_name(int cur, struct lexer_state *lexer_state);
int scan_str_literal(int cur, struct lexer_state *lexer_state);
int scan_operator(int cur, struct lexer_state *lexer_state);

void get_stream(char *input_str);
void print_tokens(struct lexer_state *lexer_state);
int find_operator(char *ops, char c);
int find_keyword(char **keywords ,char *keyword);

//string utility functions
char *my_strdup(const char *s);
#endif