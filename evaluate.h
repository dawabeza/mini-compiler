#ifndef EVALUATE_H
#define EVALUATE_H
#include <stdbool.h>

struct statement evaluate_statement(struct parser_state *parser_state);
struct statement evaluate_decl(struct parser_state *parser_state);
struct statement evaluate_block(struct parser_state *parser_state);
struct statement evaluate_if(struct parser_state *parser_state);
struct statement evaluate_for(struct parser_state *parser_state);
struct statement evaluate_while(struct parser_state *parser_state);
struct statement evaluate_expr(struct parser_state *parser_state);
struct statement evaluate_comma_separated(struct parser_state *parser_state);
struct statement evaluate_assignment(struct parser_state *parser_state);
struct statement evaluate_equality(struct parser_state *parser_state);
struct statement evaluate_comparison(struct parser_state *parser_state);
struct statement evaluate_term(struct parser_state *parser_state);
struct statement evaluate_factor(struct parser_state *parser_state);
struct statement evaluate_unary(struct parser_state *parser_state);
struct statement evaluate_basic(struct parser_state *parser_state);

#endif