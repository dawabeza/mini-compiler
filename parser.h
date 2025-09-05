#ifndef PARSER_H
#define PARSER_H
#define MAX_INPUT_EXPR 3

#include "token.h"
#include <stdbool.h>


enum statement_type {
    BLOCK_STMT,
    IF_STMT,
    FOR_STMT,
    WHILE_STMT,
    EXPR_STMT,
    COMMA_SEPARATED_STMT,
    VAR_DECL_STMT,
    FUN_DECL_STMT,
    ASSIGNMENT_STMT,
    EQUALITY_STMT,
    COMPARISON_STMT,
    TERM_STMT,
    FACTOR_STMT,
    UNARY_STMT,
    BASIC_STMT ,
    POSTFIX,
    PARAM_LIST,
    ARG_LIST,
    ERROR_EXPR, //for error_handling
    EMPTY_EXPR
};

struct statement_list {
    struct statement *data;
    int count;
    int capacity;
};

struct statement {
    struct statement_list list;
    struct token node;
    enum statement_type type;
};

struct parser_state {
    struct token_list token_list;
    int cur_token_p;
    int had_error;
};

struct lexer_state;
void init_parser_state(struct parser_state *parser_state, struct lexer_state *lexer_state);

struct token *peek_token(struct parser_state *parser_state);
struct token *advance_token(struct parser_state *parser_state);
struct token *advance_with_check(enum token_type expected_token, char *error_message, struct parser_state *parser_state);
bool parser_finished(struct parser_state *parser_state);
bool token_match(struct parser_state *parser_state, int count,  ...);

struct statement make_statement(struct parser_state * parser_state, enum statement_type type);
void destroy_statement(struct statement* stmt);
void add_child(struct statement_list *list, struct statement *new_stmt);


struct statement parse_program(struct parser_state *parser_state);
struct statement parse_statement(struct parser_state *parser_state);
struct statement parse_block(struct parser_state *parser_state);
struct statement parse_fun_decl(struct parser_state *parser_state);
struct statement parse_param_list(struct parser_state *parser_state);
struct statement parse_var_decl(struct parser_state *parser_state);
struct statement parse_if(struct parser_state *parser_state);
struct statement parse_for(struct parser_state *parser_state);
struct statement parse_while(struct parser_state *parser_state);
struct statement parse_expr_stmt(struct parser_state *parser_state);
struct statement parse_expr(struct parser_state *parser_state);
struct statement parse_assignment(struct parser_state *parser_state);
struct statement parse_equality(struct parser_state *parser_state);
struct statement parse_comparison(struct parser_state *parser_state);
struct statement parse_term(struct parser_state *parser_state);
struct statement parse_factor(struct parser_state *parser_state);
struct statement parse_unary(struct parser_state *parser_state);
struct statement parse_postfix(struct parser_state *parser_state);
struct statement parse_postfix_tail(struct parser_state *parser_state);
struct statement parse_arg_list(struct parser_state *parser_state);
struct statement parse_basic(struct parser_state *parser_state);

void synchronize(struct parser_state *parser_state);
void synchronize_block(struct parser_state *parser_state);



//utils
void pretty_printer(struct statement *stmt, int depth);
void print_node(struct statement *stmt);
#endif