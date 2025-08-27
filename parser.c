#include "parser.h"
#include <stdlib.h>

struct expr *make_expr(enum expr_type type, struct expr **child_expr, int no_child_expr, char *op, struct literal *literal)
{
    struct expr *new_expr = (struct expr *)malloc(sizeof(struct expr));
    new_expr->type = type;
    new_expr->child_expr = child_expr;
    new_expr->no_child_expr = no_child_expr;
    new_expr->op = op;
    new_expr->literal = literal;
    return new_expr;
}


void destroy_expr(struct expr *expr)
{
    for (int i = 0; i < expr->no_child_expr; i++) {
        free(expr->child_expr[i]);
    }

    free(expr->child_expr);
    free(expr->op);
    free(expr->literal);
    free(expr);
    expr->child_expr = NULL;
    expr->op = NULL;
}

struct expr *parse_unary(struct parser_state *parser)
{
    

}
