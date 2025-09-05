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

struct statement make_statement(struct parser_state * parser_state, enum statement_type type, char *node_name)
{
    struct statement stmt;
    stmt.type = type;
    stmt.node = *peek_token(parser_state);
    stmt.node_name = node_name;
    int initial_capacity = 8;
    stmt.list.data = (struct statement *)malloc(sizeof(struct statement) * initial_capacity);
    if (!stmt.list.data) {
        printf("MEMORY ALLOCATION FAILED"); exit(1);
    }
    stmt.list.count = 0;
    stmt.list.capacity = initial_capacity;
    return stmt;
}


void destroy_statement(struct statement *stmts)
{
    for (int i = 0; i < stmts->list.count; i++) {
        destroy_statement(&stmts->list.data[i]);
    }
    free(stmts->list.data);
    free(stmts->node_name);
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

struct token *advance_with_check(enum token_type expected_token, char *error_message, struct parser_state *parser_state)
{
    if (peek_token(parser_state)->token_type == TOKEN_EOF || peek_token(parser_state)->token_type != expected_token) {
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

bool parser_finished(struct parser_state *parser_state) {
    return parser_state->cur_token_p >= parser_state->token_list.count;
}


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

void add_child(struct statement_list *list, struct statement *new_stmt)
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

struct statement parse_program(struct parser_state * parser_state)
{
    struct statement full_program = make_statement(parser_state, BLOCK_STMT, strdup("FULL PROGRAM"));
    while (peek_token(parser_state)->token_type != TOKEN_EOF) {
        struct statement stmt = parse_statement(parser_state);
        if (stmt.type == ERROR_EXPR) {
            synchronize(parser_state);
        }
        add_child(&full_program.list, &stmt);
    }
    return full_program;
}

struct statement parse_statement(struct parser_state *parser_state)
{
    switch (peek_token(parser_state)->token_type) {
        case TOKEN_FUN:
            return parse_fun_decl(parser_state);
        case TOKEN_VAR:
            return parse_var_decl(parser_state);
        case TOKEN_OPEN_CURLY:
            return parse_block(parser_state);
        case TOKEN_IF:
            return parse_if(parser_state);
        case TOKEN_FOR:
            return parse_for(parser_state);
        case TOKEN_WHILE:
            return parse_while(parser_state);
        case TOKEN_EOF:
            return make_statement(parser_state, ERROR_EXPR, strdup("ERROR EXPR"));
        default:
            return parse_expr_stmt(parser_state);
    }
}

struct statement parse_block(struct parser_state *parser_state)
{
    struct statement block = make_statement(parser_state, BLOCK_STMT, strdup("BLOCK STMT"));
    advance_token(parser_state);
    while (peek_token(parser_state)->token_type != TOKEN_EOF && peek_token(parser_state)->token_type != TOKEN_CLOSED_CURLY) {
        struct statement stmt = parse_statement(parser_state);
        if (stmt.type == ERROR_EXPR) {
            synchronize_block(parser_state);
        }
        add_child(&block.list, &stmt);
    }


    if (peek_token(parser_state)->token_type != TOKEN_CLOSED_CURLY) {
        report_parse_error(parser_state, "expect '} after block statement");
        block.type = ERROR_EXPR;
        return block;
    }
    advance_token(parser_state);
    return block;
}


struct statement parse_fun_decl(struct parser_state *parser_state)
{
    struct statement fun_decl = make_statement(parser_state, FUN_DECL_STMT, strdup(peek_token(parser_state)->lexeme));
    advance_token(parser_state); // consume "fun"
    // Expect IDENTIFIER
    if (peek_token(parser_state)->token_type != TOKEN_IDENTIFIER) {
        report_parse_error(parser_state, "Expected function name after 'fun'");
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }
    struct statement name_stmt = make_statement(parser_state, BASIC_STMT, strdup(peek_token(parser_state)->lexeme));
    advance_token(parser_state);
    add_child(&fun_decl.list, &name_stmt);

    // Expect "("
    if (peek_token(parser_state)->token_type != TOKEN_OPEN_PARENTHESIS) {
        report_parse_error(parser_state, "Expected '(' after function name");
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }
    advance_token(parser_state); // consume "("

    // Optional PARAM_LIST
    if (peek_token(parser_state)->token_type == TOKEN_IDENTIFIER) {
        struct statement params = parse_param_list(parser_state);
        if (params.type == ERROR_EXPR) {
            add_child(&fun_decl.list, &params);
            fun_decl.type = ERROR_EXPR;
            return fun_decl;
        }
        add_child(&fun_decl.list, &params);
    }

    // Expect ")"
    if (peek_token(parser_state)->token_type != TOKEN_CLOSED_PARENTHESIS) {
        report_parse_error(parser_state, "Expected ')' after parameter list");
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }
    advance_token(parser_state); // consume ")"

    // Expect BLOCK_STMT
    if (peek_token(parser_state)->token_type != TOKEN_OPEN_CURLY) {
        report_parse_error(parser_state, "Expected '{' to start function body");
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }
    struct statement body = parse_block(parser_state);
    if (body.type == ERROR_EXPR) {
        add_child(&fun_decl.list, &body);
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }

    add_child(&fun_decl.list, &body);

    return fun_decl;
}

struct statement parse_param_list(struct parser_state *parser_state)
{
    struct statement param_list = make_statement(parser_state, COMMA_SEPARATED_STMT, strdup("PARAMETER LIST"));
    // Expect at least one IDENTIFIER
    if (peek_token(parser_state)->token_type != TOKEN_IDENTIFIER) {
        report_parse_error(parser_state, "Expected parameter identifier");
        param_list.type = ERROR_EXPR;
        return param_list;
    }
    // Parse first IDENTIFIER
    struct statement param = make_statement(parser_state, BASIC_STMT, strdup(peek_token(parser_state)->lexeme));
    param.node = *advance_token(parser_state);
    add_child(&param_list.list, &param);

    // Parse (',' IDENTIFIER)*
    while (peek_token(parser_state)->token_type == TOKEN_COMMA) {
        advance_token(parser_state); // consume ','
        if (peek_token(parser_state)->token_type != TOKEN_IDENTIFIER) {
            report_parse_error(parser_state, "Expected identifier after ',' in parameter list");
            param_list.type = ERROR_EXPR;
            return param_list;
        }
        struct statement next_param = make_statement(parser_state, BASIC_STMT, strdup(peek_token(parser_state)->lexeme));
        next_param.node = *advance_token(parser_state);
        add_child(&param_list.list, &next_param);
    }
    return param_list;
}

struct statement parse_var_decl(struct parser_state *parser_state)
{
    struct statement decl_stmt = make_statement(parser_state, VAR_DECL_STMT, strdup(peek_token(parser_state)->lexeme));
    advance_token(parser_state);
    if (peek_token(parser_state)->token_type ==  TOKEN_IDENTIFIER) {
        struct statement identifier = parse_basic(parser_state);
        add_child(&decl_stmt.list, &identifier);
    }
    else {
        report_parse_error(parser_state, "expect identifier after variable declarator 'var'");
        decl_stmt.type = ERROR_EXPR;
        return decl_stmt;
    }
    //optional initializer for variable declaration
    if (peek_token(parser_state)->token_type == TOKEN_ASSIGNMENT) {
        advance_token(parser_state);
        struct statement initializer = parse_expr(parser_state);
        if (initializer.type == ERROR_EXPR) {
            add_child(&decl_stmt.list, &initializer);
            decl_stmt.type = ERROR_EXPR;
            return decl_stmt;         
        }
        add_child(&decl_stmt.list, &initializer);
    }

    if (peek_token(parser_state)->token_type != TOKEN_SEMICOLON) {
        report_parse_error(parser_state, "expect ; after variable declaration");
        decl_stmt.type = ERROR_EXPR;
        return decl_stmt;
    }
    advance_token(parser_state);

    return decl_stmt;
}

struct statement parse_if(struct parser_state *parser_state)
{
    struct statement if_stmt = make_statement(parser_state, IF_STMT, strdup(peek_token(parser_state)->lexeme));
    advance_token(parser_state);
     if (peek_token(parser_state)->token_type != TOKEN_OPEN_PARENTHESIS) {
        report_parse_error(parser_state, "expect '(' after 'if' keyword");
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }
    advance_token(parser_state);
    struct statement condition_expr = parse_expr(parser_state);
    if (condition_expr.type == ERROR_EXPR) {
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }
    if (peek_token(parser_state)->token_type != TOKEN_CLOSED_PARENTHESIS) {
        report_parse_error(parser_state, "expect ')' after if expression");
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }
    advance_token(parser_state);
    struct statement stmt = parse_statement(parser_state);

    if (stmt.type == ERROR_EXPR) {
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }

    if (stmt.type == EMPTY_EXPR) {
        if_stmt.type = ERROR_EXPR;
        report_parse_error(parser_state, "if expression cannot be empty");
        return if_stmt;
    }
    
    add_child(&if_stmt.list, &condition_expr);
    add_child(&if_stmt.list, &stmt);
   if (peek_token(parser_state) && peek_token(parser_state)->token_type == TOKEN_ELSE) {
    advance_token(parser_state);
    struct statement else_stmt = parse_statement(parser_state);
    if (else_stmt.type == ERROR_EXPR) {
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }
    add_child(&if_stmt.list, &else_stmt);
   }
    return if_stmt;
}

struct statement parse_for(struct parser_state *parser_state)
{
    struct statement for_stmt = make_statement(parser_state, FOR_STMT, strdup(peek_token(parser_state)->lexeme));
    advance_token(parser_state);
    if (peek_token(parser_state)->token_type != TOKEN_OPEN_PARENTHESIS) {
        report_parse_error(parser_state, "expect '(' after 'for' keyword");
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }
    advance_token(parser_state);
    //for each child expression, if one is error, the for statement itself is error
    struct statement init_expr = parse_expr_stmt(parser_state);
    if (init_expr.type == ERROR_EXPR) {
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }
    struct statement test_expr = parse_expr_stmt(parser_state);
    if (test_expr.type == ERROR_EXPR) {
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }
    //here the third expression part of the for loop(increment) is not actually an expression since not terminated by semicolon
    struct statement increment_expr = parse_expr(parser_state);
    if (increment_expr.type == ERROR_EXPR) {
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }
     if (peek_token(parser_state)->token_type != TOKEN_CLOSED_PARENTHESIS) {
        report_parse_error(parser_state, "expect ')' after for statement expression");
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }
    advance_token(parser_state);
    struct statement stmt = parse_statement(parser_state);
    if (stmt.type == ERROR_EXPR) {
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }

    add_child(&for_stmt.list, &init_expr);
    add_child(&for_stmt.list, &test_expr);
    add_child(&for_stmt.list, &increment_expr);
    add_child(&for_stmt.list, &stmt);

    return for_stmt;
}

struct statement parse_while(struct parser_state *parser_state)
{
    struct statement while_stmt = make_statement(parser_state, WHILE_STMT, strdup(peek_token(parser_state)->lexeme));
    advance_token(parser_state);
    if (peek_token(parser_state)->token_type != TOKEN_OPEN_PARENTHESIS) {
        report_parse_error(parser_state, "expect '(' after while keyword");
       while_stmt.type = ERROR_EXPR;
       return while_stmt;
    }
    advance_token(parser_state);
    struct statement child_expr = parse_expr(parser_state);
    if (child_expr.type == ERROR_EXPR) {
        while_stmt.type = ERROR_EXPR;
        return while_stmt;
    }
    if (child_expr.type == EMPTY_EXPR) {
        report_parse_error(parser_state, "expect expression here");
        while_stmt.type == ERROR_EXPR;
        return while_stmt;
    }
    if (peek_token(parser_state)->token_type != TOKEN_CLOSED_PARENTHESIS) {
        report_parse_error(parser_state, "expect ')' after while experssion");
        while_stmt.type = ERROR_EXPR;
        return while_stmt;
    }
    advance_token(parser_state);
    struct statement child_stmt = parse_statement(parser_state);
    if (child_stmt.type == ERROR_EXPR) {
       while_stmt.type = ERROR_EXPR;
       return while_stmt; 
    }
    add_child(&while_stmt.list, &child_expr);
    add_child(&while_stmt.list, &child_stmt);
    return while_stmt;
}

struct statement parse_expr_stmt(struct parser_state *parser_state)
{
    struct statement expr_stmt = parse_expr(parser_state);
    if (expr_stmt.type == ERROR_EXPR) {
        expr_stmt.type = ERROR_EXPR;
        return expr_stmt;
    }
    if (peek_token(parser_state)->token_type != TOKEN_SEMICOLON) {
        report_parse_error(parser_state, "expect ';' after end of the expression");
        expr_stmt.type = ERROR_EXPR;
    }
    advance_token(parser_state);
    return expr_stmt;
}

struct statement parse_expr(struct parser_state *parser_state)
{
    struct statement left = parse_assignment(parser_state);
    if (left.type == ERROR_EXPR) return left;
    while (cur_token_match(",", parser_state)) {
            struct statement root = make_statement(parser_state, EQUALITY_STMT, strdup(peek_token(parser_state)->lexeme));
            advance_token(parser_state);
            struct statement right = parse_assignment(parser_state);
            if (right.type == ERROR_EXPR) return right;
            if (right.type == EMPTY_EXPR) {
                report_parse_error(parser_state, "the right operand of ',' is missing");
                right.type = ERROR_EXPR;
                return right;
            }
            add_child(&root.list, &left);
            add_child(&root.list, &right);
            left = root;
    }

    return left;
    
}

struct statement parse_assignment(struct parser_state *parser_state)
{
    struct statement left = parse_equality(parser_state);
    if (left.type == ERROR_EXPR) return left;
    if (cur_token_match("=", parser_state) || cur_token_match("+=", parser_state) || cur_token_match("-=", parser_state) ||
        cur_token_match("*=", parser_state) || cur_token_match("/=", parser_state) || cur_token_match("%=", parser_state)) {
            struct statement root = make_statement(parser_state, ASSIGNMENT_STMT, strdup(peek_token(parser_state)->lexeme));
            advance_token(parser_state);
            struct statement right = parse_assignment(parser_state);
            if (right.type == ERROR_EXPR) return right;
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                right.type = ERROR_EXPR;
                return right;
            }
            add_child(&root.list, &left);
            add_child(&root.list, &right);
            left = root;
    }

    return left;
}

struct statement parse_equality(struct parser_state * parser_state)
{
    struct statement left = parse_comparison(parser_state);
    if (left.type == ERROR_EXPR) return  left;
    while (cur_token_match("==", parser_state) || cur_token_match("!=", parser_state)) {
            struct statement root = make_statement(parser_state, EQUALITY_STMT, strdup(peek_token(parser_state)->lexeme));
            advance_token(parser_state);
            struct statement right = parse_comparison(parser_state);
            if (right.type == ERROR_EXPR) return right;
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                right.type = ERROR_EXPR;
                return right;
            }
            add_child(&root.list, &left);
            add_child(&root.list, &right);
            left = root;
    }

    return left;
    
}

struct statement parse_comparison(struct parser_state *parser_state)
{
    struct statement left = parse_term(parser_state);
    if (left.type == ERROR_EXPR) return left;
    while (cur_token_match(">", parser_state) || cur_token_match(">=", parser_state) || 
           cur_token_match("<", parser_state) || cur_token_match("<=", parser_state)) {
            struct statement root = make_statement(parser_state, COMPARISON_STMT, strdup(peek_token(parser_state)->lexeme));
            advance_token(parser_state);
            struct statement right = parse_term(parser_state);
            if (right.type == ERROR_EXPR) {
                right.type = ERROR_EXPR;
                right.node_name = strdup("ERROR EXPR");
                return right;
            }
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                right.type = ERROR_EXPR;
                return right;
            }
            add_child(&root.list, &left);
            add_child(&root.list, &right);
            left = root;
    }

    return left;
}

struct statement parse_term(struct parser_state *parser_state)
{
    struct statement left = parse_factor(parser_state);
    if (left.type == ERROR_EXPR) return left;
    while (cur_token_match("+", parser_state) || cur_token_match("-", parser_state)) {
            struct statement root = make_statement(parser_state, TERM_STMT, strdup(peek_token(parser_state)->lexeme));
            advance_token(parser_state);
            struct statement right = parse_factor(parser_state);
             if (right.type == ERROR_EXPR) {
                right.type = ERROR_EXPR;
                right.node_name = strdup("ERROR EXPR");
                return right;
            }
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                right.type = ERROR_EXPR;
                return right;
            }
            add_child(&root.list, &left);
            add_child(&root.list, &right);
            left = root;
    }

    return left;

}

struct statement parse_factor(struct parser_state* parser_state)
{
    struct statement left = parse_unary(parser_state);
    if (left.type == ERROR_EXPR) return left;
    while (cur_token_match("/", parser_state) || cur_token_match("*", parser_state) || cur_token_match("%", parser_state)) {
            struct statement root = make_statement(parser_state, FACTOR_STMT, strdup(peek_token(parser_state)->lexeme));
            advance_token(parser_state);
            struct statement right = parse_unary(parser_state);
             if (right.type == ERROR_EXPR) {
                right.type = ERROR_EXPR;
                right.node_name = strdup("ERROR EXPR");
                return right;
            }
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                right.type = ERROR_EXPR;
                return right;
            }
            add_child(&root.list, &left);
            add_child(&root.list, &right);
            left = root;
    }

    return left;
}

struct statement parse_unary(struct parser_state *parser_state)
{
    if (cur_token_match("+", parser_state) || cur_token_match("-", parser_state)) {
        struct statement stmt = make_statement(parser_state, UNARY_STMT, strdup(peek_token(parser_state)->lexeme));
        advance_token(parser_state);
        struct statement child = parse_basic(parser_state);
        if (child.type == ERROR_EXPR) return child;
        if (child.type == EMPTY_EXPR) {
            char error_buffer[40];
            sprintf(error_buffer, "the right operand of %s is missing", stmt.node.lexeme);
            report_parse_error(parser_state, error_buffer);
            return child;
        }
        add_child(&stmt.list, &child);
        return stmt;
    }

    return parse_basic(parser_state);
}

struct statement parse_basic(struct parser_state *parser_state)
{
    if (peek_token(parser_state) && peek_token(parser_state)->token_type == TOKEN_NUMBER || 
                                    peek_token(parser_state)->token_type == TOKEN_STR_LITERAL || 
                                    peek_token(parser_state)->token_type == TOKEN_IDENTIFIER) {
        struct statement  stmt = make_statement(parser_state, BASIC_STMT, strdup(peek_token(parser_state)->lexeme));
        advance_token(parser_state);
        return stmt;
    }

    if (peek_token(parser_state) && peek_token(parser_state)->token_type == TOKEN_OPEN_PARENTHESIS) {
        advance_token(parser_state);
        struct statement child = parse_expr(parser_state);
        if (child.type == ERROR_EXPR) return child;
        if (peek_token(parser_state)->token_type != TOKEN_CLOSED_PARENTHESIS) {
            report_parse_error(parser_state, "expect ')' here in the end of grouping expression");
            child.type = ERROR_EXPR;
            return child;
        }
        advance_token(parser_state);
        return child;
    }

    return make_statement(parser_state, EMPTY_EXPR, strdup("EMPTY EXPR"));
}

void synchronize(struct parser_state *parser_state)
{
    while (peek_token(parser_state)->token_type !=  TOKEN_EOF) {
        switch (peek_token(parser_state)->token_type) {
            case TOKEN_IF: case TOKEN_WHILE: case TOKEN_FOR: case TOKEN_OPEN_CURLY:
            case TOKEN_VAR: case TOKEN_FUN:
                return;
            case TOKEN_SEMICOLON: case TOKEN_CLOSED_CURLY:
                advance_token(parser_state);
                return;
            default:
                advance_token(parser_state);
        }
    }
}

void synchronize_block(struct parser_state *parser_state)
{
    while (peek_token(parser_state)->token_type != TOKEN_EOF) {
        switch (peek_token(parser_state)->token_type) {
            case TOKEN_IF: case TOKEN_WHILE: case TOKEN_FOR: case TOKEN_OPEN_CURLY: case TOKEN_VAR: 
            case TOKEN_CLOSED_CURLY: case TOKEN_FUN:
                return;
            case TOKEN_SEMICOLON:
                advance_token(parser_state);
                return;
            default:
                advance_token(parser_state);
        }
    }
}
