#ifndef PARSER_H
#define PARSER_H
#define MAX_INPUT_EXPR 3

#include "token.h"


enum expr_type {
    BINARY_EXPR,
    UNARY_EXPR, 
    GROUPING_EXPR, 
    LITERAL_EXPR,
    PRIMARY_EXPR
};

struct expr {
    struct expr **child_expr;
    int no_child_expr;
    enum expr_type type;
    char *op;
    struct literal *literal;
};

struct parser_state {
    struct token_list token_list;
    int cur_token_p;
    int had_error;
};

void init_parser_state(struct parser_state *parser_state, struct lexer_state *lexer_state);

struct expr *make_expr(struct parser_state *parser_state, enum expr_type type, int no_child_expr, char *node_lexeme);
void destroy_expr(struct expr *expr);

struct token *peek_token(struct parser_state *parser_state);
struct token *advance_token(struct parser_state *parser_state);
struct token *advance_with_check(char *expected_lexme, char *error_if_not_expected, struct parser_state *parser_state);
int cur_token_match(char *lexeme, struct parser_state *parser_state);

struct expr *parse_primary(struct parser_state *parser_state);
struct expr *parse_unary(struct parser_state *parser_state);
struct expr *parse_factor(struct parser_state *parser_state);
struct expr *parse_term(struct parser_state *parser_state);
struct expr *parse_comparison(struct parser_state *parser_state);
struct expr *parse_equality(struct parser_state *parser_state);
struct expr *parse_full_expr(struct parser_state *parser_state);



//utils
void pretty_printer(struct expr* expr, int depth);
#endif