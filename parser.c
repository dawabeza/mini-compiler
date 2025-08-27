#include "lexer.h"
#include "parser.h"
#include "error_handling.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void init_parser_state(struct parser_state *parser_state, struct lexer_state *lexer_state)
{
    parser_state->cur_token_p = 0;
    parser_state->had_error = 0;

    parser_state->token_list = lexer_state->token_list;
}

struct expr *make_expr(struct parser_state *parser_state, enum expr_type type, int no_child_expr, char *node_lexeme)
{
    struct expr *new_expr = (struct expr *)malloc(sizeof (struct expr));
    new_expr->type = type;
    new_expr->child_expr = (struct expr **)malloc(sizeof (struct expr *) * no_child_expr);
    new_expr->no_child_expr = no_child_expr;
    new_expr->op = node_lexeme;
    new_expr->literal = peek_token(parser_state)->literal;
    return new_expr;
}


void destroy_expr(struct expr *expr)
{
    for (int i = 0; i < expr->no_child_expr; i++) {
        free(expr->child_expr[i]);
    }

    free(expr->child_expr);
    free(expr);
    expr->child_expr = NULL;
    expr->op = NULL;
}

struct token *peek_token(struct parser_state *parser_state)
{
    if (parser_state->cur_token_p >= parser_state->token_list.count) {
        return NULL;
    }

    return parser_state->token_list.data[parser_state->cur_token_p];
}

struct token *advance_token(struct parser_state *parser_state)
{
    if (parser_state->cur_token_p + 1 >= parser_state->token_list.count) {
        return NULL;
    }
    return parser_state->token_list.data[parser_state->cur_token_p++];
}

struct token *advance_with_check(char *expected_lexme, char *error_if_not_expected, struct parser_state *parser_state)
{
    if (!peek_token(parser_state) || strcmp(expected_lexme, peek_token(parser_state)->lexeme) != 0) {
        report_parse_error(parser_state, error_if_not_expected);
        return NULL;
    }
    return advance_token(parser_state);
}

int cur_token_match(char *lexeme, struct parser_state *parser_state)
{
    return strcmp(lexeme, peek_token(parser_state)->lexeme) == 0;
}


struct expr *parse_primary(struct parser_state *parser_state)
{
    if (peek_token(parser_state)->token_type == TOKEN_OPEN_PARENTHESIS) {
        advance_token(parser_state);
        struct expr *nested_expr = parse_full_expr(parser_state);
        if (advance_with_check(")", "NO CLOSING PARENTHESIS", parser_state))
            return nested_expr;
    }

    struct expr *new_expr = make_expr(parser_state, PRIMARY_EXPR, 0, peek_token(parser_state)->lexeme);
    advance_token(parser_state);
    return new_expr;
}

struct expr *parse_unary(struct parser_state *parser_state)
{
    struct expr *unary_expr = make_expr(parser_state, UNARY_EXPR, 1, peek_token(parser_state)->lexeme);
    if (cur_token_match("+", parser_state) ||  cur_token_match("-", parser_state)) {
        advance_token(parser_state);
        unary_expr->child_expr[0] = parse_primary(parser_state);
        return unary_expr;
    }
    //since no pratical value
    destroy_expr(unary_expr);

    return parse_primary(parser_state);
}

struct expr *parse_factor(struct parser_state *parser_state)
{
    struct expr *left_associative_expr = parse_unary(parser_state);

    while (cur_token_match("*", parser_state) || cur_token_match("/", parser_state) || cur_token_match("%", parser_state)) {
        struct expr *temp_factor = make_expr(parser_state, BINARY_EXPR, 2, peek_token(parser_state)->lexeme);
        advance_token(parser_state);
        temp_factor->child_expr[0] = left_associative_expr;
        temp_factor->child_expr[1] = parse_unary(parser_state);
        left_associative_expr = temp_factor;
    }
    
    return left_associative_expr;
}

struct expr *parse_term(struct parser_state * parser_state)
{
    struct expr *left_associative_expr = parse_factor(parser_state);

    while (cur_token_match("+", parser_state) || cur_token_match("-", parser_state)) {
        struct expr *temp_factor = make_expr(parser_state, BINARY_EXPR, 2, peek_token(parser_state)->lexeme);
        advance_token(parser_state);
        temp_factor->child_expr[0] = left_associative_expr;
        temp_factor->child_expr[1] = parse_factor(parser_state);
        left_associative_expr = temp_factor;
    } 
    return left_associative_expr;
}

struct expr *parse_comparison(struct parser_state *parser_state)
{
    struct expr *left_associative_expr = parse_term(parser_state);

    while (cur_token_match(">", parser_state) || cur_token_match(">=", parser_state) ||
           cur_token_match("<", parser_state) || cur_token_match("<=", parser_state)) {
        struct expr *temp_factor = make_expr(parser_state, BINARY_EXPR, 2, peek_token(parser_state)->lexeme);
        advance_token(parser_state);
        temp_factor->child_expr[0] = left_associative_expr;
        temp_factor->child_expr[1] = parse_term(parser_state);
        left_associative_expr = temp_factor;
    } 
    return left_associative_expr;
}

struct expr *parse_equality(struct parser_state *parser_state)
{
    struct expr *left_associative_expr = parse_comparison(parser_state);

    while (cur_token_match("==", parser_state) || cur_token_match("!=", parser_state)) {
        struct expr *temp_factor = make_expr(parser_state, BINARY_EXPR, 2, peek_token(parser_state)->lexeme);
        advance_token(parser_state);
        temp_factor->child_expr[0] = left_associative_expr;
        temp_factor->child_expr[1] = parse_comparison(parser_state);
        left_associative_expr = temp_factor;
    } 

    return left_associative_expr;
    
}

struct expr *parse_full_expr(struct parser_state *parser_state)
{

    return parse_equality(parser_state);
}

void pretty_printer(struct expr *expr, int depth)
{
    for (int i = 0; i < depth; i++) {
        printf(" ");
    }

    printf("%s\n", expr->op);
    for (int i = 0; i < expr->no_child_expr; i++) {
        pretty_printer(expr->child_expr[i], depth + 1);
    }
}

