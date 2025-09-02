#ifndef PARSER_H
#define PARSER_H
#define MAX_INPUT_EXPR 3

#include "token.h"


enum statement_type {
    DECL_STMT,
    BLOCK_STMT,
    IF_STMT,
    FOR_STMT,
    WHILE_STMT,
    EXPR_STMT,
    COMMA_SEPARATED_STMT,
    ASSIGNMENT_STMT,
    EQUALITY_STMT,
    COMPARISON_STMT,
    TERM_STMT,
    FACTOR_STMT,
    UNARY_STMT,
    BASIC_STMT ,
    NONE,//special for error handling
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
struct token *advance_with_check(char *expected_lexme, char *error_message, struct parser_state *parser_state);
int cur_token_match(char *lexeme, struct parser_state *parser_state);

struct statement make_statement(struct parser_state * parser_state, enum statement_type type);
void destroy_statements(struct statement_list* stmts);
void add_statement(struct statement_list *list, struct statement *new_stmt);

struct statement parse_statement(struct parser_state *parser_state);
struct statement parse_decl(struct parser_state *parser_state);
struct statement parse_block(struct parser_state *parser_state);
struct statement parse_if(struct parser_state *parser_state);
struct statement parse_for(struct parser_state *parser_state);
struct statement parse_while(struct parser_state *parser_state);
struct statement parse_expr(struct parser_state *parser_state);
struct statement parse_comma_separated(struct parser_state *parser_state);
struct statement parse_assignment(struct parser_state *parser_state);
struct statement parse_equality(struct parser_state *parser_state);
struct statement parse_comparison(struct parser_state *parser_state);
struct statement parse_term(struct parser_state *parser_state);
struct statement parse_factor(struct parser_state *parser_state);
struct statement parse_unary(struct parser_state *parser_state);
struct statement parse_basic(struct parser_state *parser_state);





//utils
void pretty_printer(struct statement *stmt, int depth);
#endif