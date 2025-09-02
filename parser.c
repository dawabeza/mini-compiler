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

struct statement make_statement(struct parser_state * parser_state, enum statement_type type)
{
    struct statement stmt;
    stmt.type = type;
    stmt.node = *peek_token(parser_state);
    int initial_capacity = 8;
    stmt.list.data = (struct statement *)malloc(sizeof(struct statement) * initial_capacity);
    if (!stmt.list.data) {
        printf("MEMORY ALLOCATION FAILED"); exit(1);
    }
    stmt.list.count = 0;
    stmt.list.capacity = initial_capacity;
    return stmt;
}


void destroy_statements(struct statement_list *stmts)
{
    for (int i = 0; i < stmts->count; i++) {
        destroy_statements(&stmts->data[i].list);
    }

    free(stmts->data);
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
    if (parser_state->cur_token_p >= parser_state->token_list.count) {
        return NULL;
    }
    return parser_state->token_list.data[parser_state->cur_token_p++];
}

struct token *advance_with_check(char *expected_lexme, struct parser_state *parser_state)
{
    if (!peek_token(parser_state) || strcmp(expected_lexme, peek_token(parser_state)->lexeme) != 0) {
        return NULL;
    }
    return advance_token(parser_state);
}

int cur_token_match(char *lexeme, struct parser_state *parser_state)
{
    if (!peek_token(parser_state)) {
        return 0;
    }
    return strcmp(lexeme, peek_token(parser_state)->lexeme) == 0;
}

// struct statement parse_statement(struct parser_state *parser_state)
// {
//     switch (peek_token(parser_state)->token_type)
//     {
//     case TOKEN_VAR: return parse_decl(parser_state);
//     case TOKEN_IF: return parse_if(parser_state);
//     case TOKEN_FOR: return parse_for(parser_state);
//     case TOKEN_WHILE: return parse_while(parser_state);
//     case TOKEN_OPEN_CURLY: return parse_block(parser_state);
//     default: return parse_expr(parser_state);
//     }
    
// }


void pretty_printer(struct statement *stmt, int depth)
{
    for (int i = 0; i < depth; i++) {
        printf(" ");
    }
    if (stmt->type != NONE)
        printf("%s\n",  stmt->node.lexeme);
    for (int i = 0; i < stmt->list.count; i++) {
        pretty_printer(&stmt->list.data[i], depth + 1);
    }
}

void init_stmt_list(struct statement_list *stmt_list)
{
    stmt_list->capacity = 8;
    stmt_list->data = (struct statement *)malloc(sizeof(struct statement) * stmt_list->capacity);
    if (!stmt_list->data) {
        printf("MEMORY ALLOCATION FAILED"); exit(1);
    }
}

void add_statement(struct statement_list *list, struct statement *new_stmt)
{
    if (list->count == list->capacity) {
        list->capacity *= 2;
        list->data = (struct statement *)realloc(list->data, list->capacity);
        if (!list->data) {
            printf("MEMORY ALLOCATION FAILED"); exit(1);
        }
    }
    list->data[list->count++] = *new_stmt;
}

struct statement parse_expr(struct parser_state *parser_state)
{
    return parse_comma_separated(parser_state);
}

struct statement parse_comma_separated(struct parser_state *parser_state)
{
    struct statement left = parse_assignment(parser_state);
    while (cur_token_match(",", parser_state)) {
            struct statement root = make_statement(parser_state, EQUALITY_STMT);
            advance_token(parser_state);
            struct statement right = parse_assignment(parser_state);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;
    
}

struct statement parse_assignment(struct parser_state *parser_state)
{
   struct statement left = parse_equality(parser_state);
    if (cur_token_match("=", parser_state) || cur_token_match("+=", parser_state) || cur_token_match("-=", parser_state) ||
        cur_token_match("*=", parser_state) || cur_token_match("/=", parser_state) || cur_token_match("%=", parser_state)) {
            struct statement root = make_statement(parser_state, ASSIGNMENT_STMT);
            advance_token(parser_state);
            struct statement right = parse_assignment(parser_state);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;
}

struct statement parse_equality(struct parser_state * parser_state)
{
    struct statement left = parse_comparison(parser_state);
    while (cur_token_match("==", parser_state) || cur_token_match("!=", parser_state)) {
            struct statement root = make_statement(parser_state, EQUALITY_STMT);
            advance_token(parser_state);
            struct statement right = parse_comparison(parser_state);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;
    
}

struct statement parse_comparison(struct parser_state *parser_state)
{
    struct statement left = parse_term(parser_state);
    while (cur_token_match(">", parser_state) || cur_token_match(">=", parser_state) || 
           cur_token_match("<", parser_state) || cur_token_match("<=", parser_state)) {
            struct statement root = make_statement(parser_state, COMPARISON_STMT);
            advance_token(parser_state);
            struct statement right = parse_term(parser_state);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;
}

struct statement parse_term(struct parser_state *parser_state)
{
    struct statement left = parse_factor(parser_state);
    while (cur_token_match("+", parser_state) || cur_token_match("-", parser_state)) {
            struct statement root = make_statement(parser_state, TERM_STMT);
            advance_token(parser_state);
            struct statement right = parse_factor(parser_state);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;

}

struct statement parse_factor(struct parser_state* parser_state)
{
    struct statement left = parse_unary(parser_state);
    while (cur_token_match("/", parser_state) || cur_token_match("*", parser_state) || cur_token_match("%", parser_state)) {
            struct statement root = make_statement(parser_state, FACTOR_STMT);
            advance_token(parser_state);
            struct statement right = parse_unary(parser_state);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;
}

struct statement parse_unary(struct parser_state *parser_state)
{
    if (cur_token_match("+", parser_state) || cur_token_match("-", parser_state)) {
        struct statement stmt = make_statement(parser_state, UNARY_STMT);
        advance_token(parser_state);
        struct statement child = parse_basic(parser_state);
        add_statement(&stmt.list, &child);
        return stmt;
    }

    return parse_basic(parser_state);
}

struct statement parse_basic(struct parser_state *parser_state)
{
    if (peek_token(parser_state)->token_type == TOKEN_NUMBER || peek_token(parser_state)->token_type == TOKEN_STR_LITERAL ||
        peek_token(parser_state)->token_type == TOKEN_IDENTIFIER) {
        struct statement  stmt = make_statement(parser_state, BASIC_STMT);
        advance_token(parser_state);
        return stmt;
    }

    if (peek_token(parser_state)->token_type == TOKEN_OPEN_PARENTHESIS) {
        struct statement stmt = make_statement(parser_state, BASIC_STMT);
        advance_token(parser_state);
        struct statement child = parse_expr(parser_state);
        add_statement(&stmt.list, &child);
        if (advance_with_check(")", parser_state) == NULL) {
            report_parse_error(parser_state, "NO CLOSING PARENTHESIS");
            return make_statement(parser_state, NONE);
        }
        return stmt;
    }

    return make_statement(parser_state, NONE);
}
