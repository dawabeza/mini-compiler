#ifndef PARSER_H
#define PARSER_H
#define MAX_INPUT_EXPR 3

#include "token.h"


enum expr_type {
    BINARY_EXPR,
    UNARY_EXPR, 
    GROUPING_EXPR, 
    LITERAL_EXPR
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
    int cur_token;
    int had_error;
};

struct expr *make_expr(enum expr_type type, struct expr **chile_expr, int no_child_expr, char *op, struct literal *literal_val);
void destroy_expr(struct expr *expr);

struct expr *parse_unary(struct parser_state *parser);
#endif