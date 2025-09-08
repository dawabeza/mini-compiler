#ifndef PARSER_H
#define PARSER_H
#define MAX_INPUT_EXPR 3

#include "token.h"
#include <stdbool.h>


enum statement_type {
    FULL_PROGRAM,
    BLOCK_STMT,
    IF_STMT,
    FOR_STMT,
    WHILE_STMT,
    EXPR_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
    RETURN_STMT,
    PRINT_STMT,
    COMMA_SEPARATED_STMT,
    VAR_DECL_STMT,
    FUN_DECL_STMT,
    ASSIGNMENT_STMT,
    CONDITIONAL_STMT,
    LOGICAL_OR_STMT,
    LOGICAL_AND_STMT,
    BITWISE_OR_STMT,
    BITWISE_XOR_STMT,
    BITWISE_AND_STMT,
    EQUALITY_STMT,
    COMPARISON_STMT,
    SHIFT_STMT,
    TERM_STMT,
    FACTOR_STMT,
    UNARY_STMT,
    BASIC_STMT ,
    POSTFIX,
    INDEXING_STMT,
    MEMBER_STMT,
    POSTFIX_UNARY_STMT,
    PARAM_LIST,
    ARG_LIST,
    ERROR_EXPR, //for error_handling
    EMPTY_EXPR
};

struct statement_list {
    struct statement **data;
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
bool token_match(struct parser_state *parser_state, int count,  ...);

struct statement *make_statement(struct parser_state * parser_state, enum statement_type type);
void destroy_statement(struct statement* stmt);
void add_child(struct statement_list *list, struct statement *new_stmt);


struct statement *parse_program(struct parser_state *parser_state);
struct statement *parse_statement(struct parser_state *parser_state);
struct statement *parse_block(struct parser_state *parser_state);
struct statement *parse_fun_decl(struct parser_state *parser_state);
struct statement *parse_param_list(struct parser_state *parser_state);
struct statement *parse_var_decl(struct parser_state *parser_state);
struct statement *parse_if(struct parser_state *parser_state);
struct statement *parse_for(struct parser_state *parser_state);
struct statement *parse_while(struct parser_state *parser_state);
struct statement *parse_expr_stmt(struct parser_state *parser_state);
struct statement *parse_expr(struct parser_state *parser_state);
struct statement *parse_assignment(struct parser_state *parser_state);
struct statement *parse_conditional(struct parser_state *parser_state);
struct statement *parse_logical_or(struct parser_state *parser_state);
struct statement *parse_logical_and(struct parser_state *parser_state);
struct statement *parse_bitwise_or(struct parser_state *parser_state);
struct statement *parse_bitwise_xor(struct parser_state *parser_state);
struct statement *parse_bitwise_and(struct parser_state *parser_state);
struct statement *parse_equality(struct parser_state *parser_state);
struct statement *parse_comparison(struct parser_state *parser_state);
struct statement *parse_shift(struct parser_state *parser_state);
struct statement *parse_term(struct parser_state *parser_state);
struct statement *parse_factor(struct parser_state *parser_state);
struct statement *parse_unary(struct parser_state *parser_state);
struct statement *parse_postfix(struct parser_state *parser_state);
struct statement *parse_postfix_tail(struct parser_state *parser_state);
struct statement *parse_arg_list(struct parser_state *parser_state);
struct statement *parse_basic(struct parser_state *parser_state);
struct statement *parse_break(struct parser_state *parser_state);
struct statement *parse_continue(struct parser_state *parser_state);
struct statement *parse_return(struct parser_state *parser_state);
struct statement *parse_print(struct parser_state *parser_state);

void synchronize(struct parser_state *parser_state);
void synchronize_block(struct parser_state *parser_state);



//utils
char *node_name(struct statement *stmt);
#endif