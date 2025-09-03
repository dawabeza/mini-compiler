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

struct token *advance_with_check(char *expected_lexme, char *error_message, struct parser_state *parser_state)
{
    if (peek_token(parser_state)->token_type == TOKEN_EOF || strcmp(expected_lexme, peek_token(parser_state)->lexeme) != 0) {
        report_parse_error(parser_state, error_message);
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
    if (stmt->type != ERROR_EXPR)
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

struct statement parse_statement(struct parser_state *parser_state)
{
    return  parse_expr(parser_state);
}

struct statement parse_if(struct parser_state *parser_state)
{
    return statement();
}

struct statement parse_for(struct parser_state *parser_state)
{
    struct statement for_stmt = make_statement(parser_state, FOR_STMT);
    advance_token(parser_state);
    if (!advance_with_check("(", "INVALID PARENTHESIS", parser_state)) {
        return make_statement(parser_state, ERROR_EXPR);
    }
    //for each child expression, if one is error, the for statement itself is error
    struct statement init_expr = parse_expr(parser_state);
    if (init_expr.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
    struct statement test_expr = parse_expr(parser_state);
    if (test_expr.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
    //here the third expression part of the for loop(increment) is not actually an expression since not terminated by semicolon
    struct statement increment_expr = parse_comma_separated(parser_state);
    if (increment_expr.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
     if (!advance_with_check(")", "INVALID PARENTHESIS", parser_state)) {
        return make_statement(parser_state, ERROR_EXPR);
    }
    struct statement stmt = parse_statement(parser_state);
    if (stmt.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);

    add_statement(&for_stmt.list, &init_expr);
    add_statement(&for_stmt.list, &test_expr);
    add_statement(&for_stmt.list, &increment_expr);
    add_statement(&for_stmt.list, &stmt);

    return for_stmt;
}

struct statement parse_while(struct parser_state *parser_state)
{
    struct statement while_stmt = make_statement(parser_state, WHILE_STMT);
    advance_token(parser_state);
    if (!advance_with_check("(", "INVALID PARENTHESIS", parser_state)) {
        return make_statement(parser_state, ERROR_EXPR);
    }
    struct statement child_expr = parse_comma_separated(parser_state);
    if (child_expr.type == ERROR_EXPR || child_expr.type == EMPTY_EXPR) {
        report_parse_error(parser_state, "INVALID WHILE EXPRESSION");
        return make_statement(parser_state, ERROR_EXPR);
    }
    if (!advance_with_check(")", "INVALID PARENTHESIS", parser_state)) {
        return make_statement(parser_state, ERROR_EXPR);
    }
    struct statement child_stmt = parse_statement(parser_state);
    if (child_stmt.type == ERROR_EXPR) {
        report_parse_error(parser_state, "INVALID WHILE STATEMEN");
        return make_statement(parser_state, ERROR_EXPR);
    }
    add_statement(&while_stmt.list, &child_expr);
    add_statement(&while_stmt.list, &child_stmt);
    return while_stmt;
}

struct statement parse_expr(struct parser_state *parser_state)
{
    struct statement expr_stmt = parse_comma_separated(parser_state);
    if (!advance_with_check(";", "NO CLOSING SEMICOLONG", parser_state)) {
        return make_statement(parser_state, ERROR_EXPR);
    }
    return expr_stmt;
}

struct statement parse_comma_separated(struct parser_state *parser_state)
{
    struct statement left = parse_assignment(parser_state);
    if (left.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
    while (cur_token_match(",", parser_state)) {
            struct statement root = make_statement(parser_state, EQUALITY_STMT);
            advance_token(parser_state);
            struct statement right = parse_assignment(parser_state);
            if (right.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;
    
}

struct statement parse_assignment(struct parser_state *parser_state)
{
    struct statement left = parse_equality(parser_state);
    if (left.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
    if (cur_token_match("=", parser_state) || cur_token_match("+=", parser_state) || cur_token_match("-=", parser_state) ||
        cur_token_match("*=", parser_state) || cur_token_match("/=", parser_state) || cur_token_match("%=", parser_state)) {
            struct statement root = make_statement(parser_state, ASSIGNMENT_STMT);
            advance_token(parser_state);
            struct statement right = parse_assignment(parser_state);
            if (right.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;
}

struct statement parse_equality(struct parser_state * parser_state)
{
    struct statement left = parse_comparison(parser_state);
    if (left.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
    while (cur_token_match("==", parser_state) || cur_token_match("!=", parser_state)) {
            struct statement root = make_statement(parser_state, EQUALITY_STMT);
            advance_token(parser_state);
            struct statement right = parse_comparison(parser_state);
            if (right.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;
    
}

struct statement parse_comparison(struct parser_state *parser_state)
{
    struct statement left = parse_term(parser_state);
    if (left.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
    while (cur_token_match(">", parser_state) || cur_token_match(">=", parser_state) || 
           cur_token_match("<", parser_state) || cur_token_match("<=", parser_state)) {
            struct statement root = make_statement(parser_state, COMPARISON_STMT);
            advance_token(parser_state);
            struct statement right = parse_term(parser_state);
            if (right.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;
}

struct statement parse_term(struct parser_state *parser_state)
{
    struct statement left = parse_factor(parser_state);
    if (left.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
    while (cur_token_match("+", parser_state) || cur_token_match("-", parser_state)) {
            struct statement root = make_statement(parser_state, TERM_STMT);
            advance_token(parser_state);
            struct statement right = parse_factor(parser_state);
            if (right.type == ERROR_EXPR) make_statement(parser_state, ERROR_EXPR);
            add_statement(&root.list, &left);
            add_statement(&root.list, &right);
            left = root;
    }

    return left;

}

struct statement parse_factor(struct parser_state* parser_state)
{
    struct statement left = parse_unary(parser_state);
    if (left.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
    while (cur_token_match("/", parser_state) || cur_token_match("*", parser_state) || cur_token_match("%", parser_state)) {
            struct statement root = make_statement(parser_state, FACTOR_STMT);
            advance_token(parser_state);
            struct statement right = parse_unary(parser_state);
            if (left.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
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
        if (child.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
        add_statement(&stmt.list, &child);
        return stmt;
    }

    return parse_basic(parser_state);
}

struct statement parse_basic(struct parser_state *parser_state)
{
    if (peek_token(parser_state) && peek_token(parser_state)->token_type == TOKEN_NUMBER || 
                                    peek_token(parser_state)->token_type == TOKEN_STR_LITERAL || 
                                    peek_token(parser_state)->token_type == TOKEN_IDENTIFIER) {
        struct statement  stmt = make_statement(parser_state, BASIC_STMT);
        advance_token(parser_state);
        return stmt;
    }

    if (peek_token(parser_state) && peek_token(parser_state)->token_type == TOKEN_OPEN_PARENTHESIS) {
        advance_token(parser_state);
        struct statement child = parse_comma_separated(parser_state);
        if (child.type == ERROR_EXPR) return make_statement(parser_state, ERROR_EXPR);
        if (!advance_with_check(")", "NO CLOSING PARENTHESIS", parser_state)) {
            return make_statement(parser_state, ERROR_EXPR);
        }
        return child;
    }

    return make_statement(parser_state, EMPTY_EXPR);
}
