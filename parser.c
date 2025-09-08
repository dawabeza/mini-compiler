#include "lexer.h"
#include "parser.h"
#include "error_handling.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

void init_parser_state(struct parser_state *parser_state, struct lexer_state *lexer_state)
{
    parser_state->cur_token_p = 0;
    parser_state->had_error = 0;
    parser_state->token_list = lexer_state->token_list;
}

// NOTE: This function now returns a POINTER to the statement
struct statement *make_statement(struct parser_state * parser_state, enum statement_type type)
{
    struct statement *stmt = (struct statement *)malloc(sizeof(struct statement));
    if (!stmt) {
        printf("MEMORY ALLOCATION FAILED");
        exit(1);
    }
    stmt->type = type;
    struct token *current_token = peek_token(parser_state);
    if (current_token) {
        stmt->node = *current_token;
    } else {
        // Handle case where we're at EOF or something
        memset(&stmt->node, 0, sizeof(struct token));
    }
    
    int initial_capacity = 8;
    stmt->list.data = (struct statement **)malloc(sizeof(struct statement *) * initial_capacity);
    if (!stmt->list.data) {
        free(stmt); // Clean up the statement itself
        printf("MEMORY ALLOCATION FAILED");
        exit(1);
    }
    stmt->list.count = 0;
    stmt->list.capacity = initial_capacity;
    
    return stmt;
}

void destroy_statement(struct statement *stmts)
{
    if (!stmts) return; // safety check
    
    for (int i = 0; i < stmts->list.count; i++) {
        destroy_statement(stmts->list.data[i]);
    }
    if (stmts->list.data) {
        free(stmts->list.data);
        stmts->list.data = NULL;
    }
    free(stmts);
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

bool token_match(struct parser_state *parser_state, int count,  ...)
{
    va_list args;
    va_start(args, count);
    enum token_type type;
    struct token *cur = peek_token(parser_state);
    if (!cur) {
        va_end(args);
        return false;
    }
    for (int i = 0; i < count; i++) {
        type = va_arg(args, enum token_type);
        if (type == cur->token_type) {
            va_end(args);
            return true;
        }
    }
    va_end(args);
    return false;
}


char  *node_name(struct statement *stmt) {
    if (!stmt) return strdup("NULL");
    switch (stmt->type) {
        case PARAM_LIST: return strdup("PARAM LIST");
        case ARG_LIST: return strdup("ARG LIST");
        case BLOCK_STMT:  return strdup("BLOCK STMT");
        case IF_STMT: return strdup("IF");
        case FOR_STMT: return strdup("FOR");
        case WHILE_STMT: return strdup("WHILE");
        case POSTFIX: return strdup("POSTFIX");
        case CONDITIONAL_STMT: return strdup("CONDITIONAL STMT");
        case FULL_PROGRAM: return strdup("FULL PROGRAM");
        default:
            return strdup(stmt->node.lexeme);
    }
}

void init_stmt_list(struct statement_list *stmt_list)
{
    stmt_list->capacity = 8;
    stmt_list->data = (struct statement **)malloc(sizeof(struct statement *) * stmt_list->capacity);
    if (!stmt_list->data) {
        printf("MEMORY ALLOCATION FAILED");
        exit(1);
    }
    stmt_list->count = 0;
}

void add_child(struct statement_list *list, struct statement *new_stmt)
{
    if (!new_stmt) return; // Do not add NULL children
    if (list->count == list->capacity) {
        list->capacity *= 2;
        list->data = (struct statement **)realloc(list->data, sizeof(struct statement *) * list->capacity);
        if (!list->data) {
            printf("MEMORY ALLOCATION FAILED");
            exit(1);
        }
    }
    list->data[list->count++] = new_stmt;
}

struct statement *parse_program(struct parser_state *parser_state)
{
    struct statement *full_program = make_statement(parser_state, FULL_PROGRAM);
    while (!token_match(parser_state, 1, TOKEN_EOF)) {
        struct statement *stmt = parse_statement(parser_state);
        if (stmt == NULL) {
            synchronize(parser_state);
        }
        else  {
        add_child(&full_program->list, stmt);
        }
    }
    return full_program;
}

struct statement *parse_statement(struct parser_state *parser_state)
{
    struct token *current_token = peek_token(parser_state);
    if (!current_token) return NULL; // End of file
    
    switch (current_token->token_type) {
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
        case TOKEN_RETURN:
            return parse_return(parser_state);
        case TOKEN_PRINT:
            return parse_print(parser_state);
        case TOKEN_CONTINUE:
            return parse_continue(parser_state);
        case TOKEN_BREAK:
            return parse_break(parser_state);
        default:
            return parse_expr_stmt(parser_state);
    }
}

struct statement *parse_block(struct parser_state *parser_state)
{
    struct statement *block = make_statement(parser_state, BLOCK_STMT);
    if (!block) return NULL;
    advance_token(parser_state);
    while (peek_token(parser_state) && !token_match(parser_state, 1, TOKEN_CLOSED_CURLY)) {
        struct statement *stmt = parse_statement(parser_state);
        if (stmt == NULL) {
            synchronize_block(parser_state);
        }
        else {
            add_child(&block->list, stmt);
        }
    }

    if (!token_match(parser_state, 1, TOKEN_CLOSED_CURLY)) {
        report_parse_error(parser_state, "expect '} after block statement");
        destroy_statement(block);
        return NULL;
    }
    advance_token(parser_state);
    return block;
}


struct statement *parse_fun_decl(struct parser_state *parser_state)
{
    struct statement *fun_decl = make_statement(parser_state, FUN_DECL_STMT);
    if (!fun_decl) return NULL;
    advance_token(parser_state); // consume "fun"
    
    if (!token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
        report_parse_error(parser_state, "Expected function name after 'fun'");
        destroy_statement(fun_decl);
        return NULL;
    }
    
    struct statement *name = parse_basic(parser_state);
    if (!name) {
        destroy_statement(fun_decl);
        return NULL;
    }
    add_child(&fun_decl->list, name);
    
    if (!token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        report_parse_error(parser_state, "Expected '(' after function name");
        destroy_statement(fun_decl);
        return NULL;
    }

    advance_token(parser_state);
    
    if (token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
        struct statement *params = parse_param_list(parser_state);
        if (!params) {
            destroy_statement(fun_decl);
            return NULL;
        }
        add_child(&fun_decl->list, params);
    }
    
    if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
        report_parse_error(parser_state, "Expected ')' after parameter list");
        destroy_statement(fun_decl);
        return NULL;
    }
    advance_token(parser_state);

    if (!token_match(parser_state, 1, TOKEN_OPEN_CURLY)) {
        report_parse_error(parser_state, "Expected '{' to start function body");
        destroy_statement(fun_decl);
        return NULL;
    }
    struct statement *body = parse_block(parser_state);
    if (!body) {
        destroy_statement(fun_decl);
        return NULL;
    }
    add_child(&fun_decl->list, body);

    return fun_decl;
}

struct statement *parse_param_list(struct parser_state *parser_state)
{
    struct statement *param_list = make_statement(parser_state, PARAM_LIST);
    if (!param_list) return NULL;

    if (!token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
        report_parse_error(parser_state, "Expected parameter identifier");
        destroy_statement(param_list);
        return NULL;
    }
    
    struct statement *param = parse_basic(parser_state);
    if (!param) {
        destroy_statement(param_list);
        return NULL;
    }
    add_child(&param_list->list, param);

    while (token_match(parser_state, 1, TOKEN_COMMA)) {
        advance_token(parser_state);
        if (!token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
            report_parse_error(parser_state, "Expected identifier after ',' in parameter list");
            destroy_statement(param_list);
            return NULL;
        }
        struct statement *next_param = parse_basic(parser_state);
        if (!next_param) {
            destroy_statement(param_list);
            return NULL;
        }
        add_child(&param_list->list, next_param);
    }
    return param_list;
}

struct statement *parse_var_decl(struct parser_state *parser_state)
{
    struct statement *decl_stmt = make_statement(parser_state, VAR_DECL_STMT);
    if (!decl_stmt) return NULL;
    advance_token(parser_state);
    
    if (!token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
        report_parse_error(parser_state, "expect identifier after variable declarator 'var'");
        destroy_statement(decl_stmt);
        return NULL;
    }
    
    struct statement *identifier = parse_basic(parser_state);
    if (!identifier) {
        destroy_statement(decl_stmt);
        return NULL;
    }
    add_child(&decl_stmt->list, identifier);
    
    if (token_match(parser_state, 1, TOKEN_ASSIGNMENT)) {
        advance_token(parser_state);
        struct statement *initializer = parse_expr(parser_state);
        
        if (!initializer) {
            report_parse_error(parser_state, "variable initializer cannot be empty or invalid");
            destroy_statement(decl_stmt);
            return NULL;         
        }
        add_child(&decl_stmt->list, initializer);
    }

    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        report_parse_error(parser_state, "expect ; after variable declaration");
        destroy_statement(decl_stmt);
        return NULL;
    }
    advance_token(parser_state);

    return decl_stmt;
}

struct statement *parse_if(struct parser_state *parser_state)
{
    struct statement *if_stmt = make_statement(parser_state, IF_STMT);
    if (!if_stmt) return NULL;
    advance_token(parser_state);
    
    if (!token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        report_parse_error(parser_state, "expect '(' after 'if' keyword");
        destroy_statement(if_stmt);
        return NULL;
    }
    advance_token(parser_state);
    
    struct statement *condition_expr = parse_expr(parser_state);
    
    if (!condition_expr) {
        report_parse_error(parser_state, "if condition cannot be empty or invalid");
        destroy_statement(if_stmt);
        return NULL;
    }
    add_child(&if_stmt->list, condition_expr);
    
    if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
        report_parse_error(parser_state, "expect ')' after if expression");
        destroy_statement(if_stmt);
        return NULL;
    }
    advance_token(parser_state);
    
    struct statement *stmt = parse_statement(parser_state);

    if (!stmt) {
        report_parse_error(parser_state, "if expression cannot be empty or invalid");
        destroy_statement(if_stmt);
        return NULL;
    }
    add_child(&if_stmt->list, stmt);
    
    if (token_match(parser_state, 1, TOKEN_ELSE)) {
        advance_token(parser_state);
        struct statement *else_stmt = parse_statement(parser_state);
        
        if (!else_stmt) {
            destroy_statement(if_stmt);
            return NULL;
        }
        add_child(&if_stmt->list, else_stmt);
    }
    return if_stmt;
}

struct statement *parse_for(struct parser_state *parser_state)
{
    struct statement *for_stmt = make_statement(parser_state, FOR_STMT);
    if (!for_stmt) return NULL;
    advance_token(parser_state);
    
    if (!token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        report_parse_error(parser_state, "expect '(' after 'for' keyword");
        destroy_statement(for_stmt);
        return NULL;
    }
    advance_token(parser_state);
    
    struct statement *init_expr = parse_expr(parser_state);
    if (!init_expr) {
        destroy_statement(for_stmt);
        return NULL;
    }
    add_child(&for_stmt->list, init_expr);
    
    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        report_parse_error(parser_state, "expect ';' after for statement initialization");
        destroy_statement(for_stmt);
        return NULL;
    }
    advance_token(parser_state);
    
    struct statement *test_expr = parse_expr(parser_state);
    if (!test_expr) {
        destroy_statement(for_stmt);
        return NULL;
    }
    add_child(&for_stmt->list, test_expr);
    
    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        report_parse_error(parser_state, "expect ';' after for statement condition");
        destroy_statement(for_stmt);
        return NULL;
    }
    advance_token(parser_state);
    
    struct statement *increment_expr = parse_expr(parser_state);
    if (!increment_expr) {
        destroy_statement(for_stmt);
        return NULL;
    }
    add_child(&for_stmt->list, increment_expr);
    
    if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
        report_parse_error(parser_state, "expect ')' after for statement expression");
        destroy_statement(for_stmt);
        return NULL;
    }
    advance_token(parser_state);
    
    struct statement *stmt = parse_statement(parser_state);
    if (!stmt) {
        destroy_statement(for_stmt);
        return NULL;
    }
    add_child(&for_stmt->list, stmt);
    
    return for_stmt;
}

struct statement *parse_while(struct parser_state *parser_state)
{
    struct statement *while_stmt = make_statement(parser_state, WHILE_STMT);
    if (!while_stmt) return NULL;
    advance_token(parser_state);

    if (!token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        report_parse_error(parser_state, "expect '(' after while keyword");
        destroy_statement(while_stmt); 
        return NULL;
    }

    advance_token(parser_state);
    struct statement *child_expr = parse_expr(parser_state);
    
    if (!child_expr) {
        report_parse_error(parser_state, "expect expression here");
        destroy_statement(while_stmt); 
        return NULL;
    }
    add_child(&while_stmt->list, child_expr);

    if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
        report_parse_error(parser_state, "expect ')' after while expression");
        destroy_statement(while_stmt); 
        return NULL;
    }

    advance_token(parser_state);
    struct statement *child_stmt = parse_statement(parser_state);
    
    if (!child_stmt) {
        destroy_statement(while_stmt); 
        return NULL;
    }
    add_child(&while_stmt->list, child_stmt);

    return while_stmt;
}

struct statement *parse_expr_stmt(struct parser_state *parser_state)
{
    struct statement *expr_stmt = parse_expr(parser_state);
    if (!expr_stmt) return NULL;
    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        report_parse_error(parser_state, "expect ';' after end of the expression");
        destroy_statement(expr_stmt);
        return NULL;
    }
    advance_token(parser_state);
    return expr_stmt;
}

struct statement *parse_expr(struct parser_state *parser_state)
{
    struct statement *left = parse_assignment(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 1, TOKEN_COMMA)) {
        struct statement *root = make_statement(parser_state, EQUALITY_STMT);
        if (!root) { destroy_statement(left); return NULL; }
        advance_token(parser_state);
        struct statement *right = parse_assignment(parser_state);
        
        if (!right) {
            report_parse_error(parser_state, "Expect expression after comma");
            destroy_statement(root);
            destroy_statement(left);
            return NULL;
        }
        add_child(&root->list, left);
        add_child(&root->list, right);
        left = root;
    }
    return left;
}

struct statement *parse_assignment(struct parser_state *parser_state)
{
    struct statement *left = parse_conditional(parser_state);
    if (!left) return NULL;
    if (token_match(parser_state, 11, TOKEN_ASSIGNMENT, TOKEN_PLUS_ASSIGN, TOKEN_MINUS_ASSIGN,
                                     TOKEN_STAR_ASSIGN, TOKEN_SLASH_ASSIGN, TOKEN_MODULO_ASSIGN,
                                      TOKEN_LEFT_SHIFT_ASSIGN, TOKEN_RIGHT_SHIFT_ASSIGN,
                                       TOKEN_AND_ASSIGN, TOKEN_OR_ASSIGN, TOKEN_XOR_ASSIGN)) {
            struct statement *root = make_statement(parser_state, ASSIGNMENT_STMT);
            if (!root) { destroy_statement(left); return NULL; }
            advance_token(parser_state);
            struct statement *right = parse_assignment(parser_state);
            
            if (!right) {
                char error_buffer[40];
                sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
                report_parse_error(parser_state, error_buffer);
                destroy_statement(root);
                destroy_statement(left);
                return NULL;
            }
            add_child(&root->list, left);
            add_child(&root->list, right);
            left = root;
    }
    return left;
}

struct statement *parse_equality(struct parser_state * parser_state)
{
    struct statement *left = parse_comparison(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 2, TOKEN_EQUAL, TOKEN_BANG_EQUAL)) {
            struct statement *root = make_statement(parser_state, EQUALITY_STMT);
            if (!root) { destroy_statement(left); return NULL; }
            advance_token(parser_state);
            struct statement *right = parse_comparison(parser_state);
            
            if (!right) {
                char error_buffer[40];
                sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
                report_parse_error(parser_state, error_buffer);
                destroy_statement(root);
                destroy_statement(left);
                return NULL;
            }
            add_child(&root->list, left);
            add_child(&root->list, right);
            left = root;
    }
    return left;
}

struct statement *parse_comparison(struct parser_state *parser_state)
{
    struct statement *left = parse_shift(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 4, TOKEN_GREATER_THAN, TOKEN_GREATER_EQUAL, TOKEN_LESS_THAN, TOKEN_LESS_EQUAL)) {
            struct statement *root = make_statement(parser_state, COMPARISON_STMT);
            if (!root) { destroy_statement(left); return NULL; }
            advance_token(parser_state);
            struct statement *right = parse_shift(parser_state);
            
           if (!right) {
                char error_buffer[40];
                sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
                report_parse_error(parser_state, error_buffer);
                destroy_statement(root);
                destroy_statement(left);
                return NULL;
            }
            add_child(&root->list, left);
            add_child(&root->list, right);
            left = root;
    }
    return left;
}

struct statement *parse_shift(struct parser_state *parser_state) {
    struct statement *left = parse_term(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 2, TOKEN_LEFT_SHIFT, TOKEN_RIGHT_SHIFT)) {
        struct statement *root = make_statement(parser_state, SHIFT_STMT);
        if (!root) { destroy_statement(left); return NULL; }
        advance_token(parser_state);
        struct statement *right = parse_term(parser_state);
        
        if (!right) {
            char error_buffer[40];
            sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
            report_parse_error(parser_state, error_buffer);
            destroy_statement(root);
            destroy_statement(left);
            return NULL;
        }
        add_child(&root->list, left);
        add_child(&root->list, right);
        left = root;
    }
    return left;
}

struct statement *parse_term(struct parser_state *parser_state)
{
    struct statement *left = parse_factor(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 2, TOKEN_PLUS, TOKEN_MINUS)) {
            struct statement *root = make_statement(parser_state, TERM_STMT);
            if (!root) { destroy_statement(left); return NULL; }
            advance_token(parser_state);
            struct statement *right = parse_factor(parser_state);
            
             if (!right) {
                char error_buffer[40];
                sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
                report_parse_error(parser_state, error_buffer);
                destroy_statement(root);
                destroy_statement(left);
                return NULL;
            }
           add_child(&root->list, left);
           add_child(&root->list, right);
            left = root;
    }
    return left;
}

struct statement *parse_factor(struct parser_state* parser_state)
{
    struct statement *left = parse_unary(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 3, TOKEN_SLASH, TOKEN_STAR, TOKEN_MODULO)) {
            struct statement *root = make_statement(parser_state, FACTOR_STMT);
            if (!root) { destroy_statement(left); return NULL; }
            advance_token(parser_state);
            struct statement *right = parse_unary(parser_state);
            
             if (!right) {
                char error_buffer[40];
                sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
                report_parse_error(parser_state, error_buffer);
                destroy_statement(root);
                destroy_statement(left);
                return NULL;
            }
           add_child(&root->list, left);
           add_child(&root->list, right);
            left = root;
    }
    return left;
}

struct statement *parse_unary(struct parser_state *parser_state)
{
    if (token_match(parser_state, 6, TOKEN_BANG, TOKEN_TILDE, TOKEN_INCREMENT, TOKEN_DECREMENT, TOKEN_PLUS, TOKEN_MINUS)) {
        struct statement *stmt = make_statement(parser_state, UNARY_STMT);
        if (!stmt) return NULL;
        advance_token(parser_state);
        struct statement *child = parse_unary(parser_state);
        
        if (!child) {
            char error_buffer[40];
            sprintf(error_buffer, "the operand of %s is missing", stmt->node.lexeme);
            report_parse_error(parser_state, error_buffer);
            destroy_statement(stmt);
            return NULL;
        }
        add_child(&stmt->list, child);
        return stmt;
    }
    return parse_postfix(parser_state);
}

struct statement *parse_postfix(struct parser_state * parser_state)
{
    struct statement *root = parse_basic(parser_state);
    if (!root) return NULL;

    while (token_match(parser_state, 5, TOKEN_OPEN_PARENTHESIS, TOKEN_OPEN_BRACKET, TOKEN_DOT, TOKEN_INCREMENT, TOKEN_DECREMENT)) {
        struct statement *new_root = make_statement(parser_state, POSTFIX);
        if (!new_root) { destroy_statement(root); return NULL; }
        struct statement *tail = parse_postfix_tail(parser_state);
        
        if (!tail) {
            destroy_statement(new_root);
            destroy_statement(root);
            return NULL;
        }
        add_child(&new_root->list, root);
        add_child(&new_root->list, tail);
        root = new_root;
    }
    return root;
}

struct statement *parse_postfix_tail(struct parser_state * parser_state)
{
    if (token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        struct statement *arg_list_node = make_statement(parser_state, ARG_LIST);
        if (!arg_list_node) return NULL;
        advance_token(parser_state);
        
        struct statement *arg_list = parse_arg_list(parser_state);
        if (!arg_list) {
            destroy_statement(arg_list_node);
            return NULL;
        }
        add_child(&arg_list_node->list, arg_list);

        if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
            report_parse_error(parser_state, "expect ')' after function call");
            destroy_statement(arg_list_node);
            return NULL;
        }
        advance_token(parser_state);
        return arg_list_node;
    }

    if (token_match(parser_state, 1, TOKEN_OPEN_BRACKET)) {
        struct statement *indexing_stmt = make_statement(parser_state, INDEXING_STMT);
        if (!indexing_stmt) return NULL;
        advance_token(parser_state);
        struct statement *expr = parse_expr(parser_state);
        
        if (!expr) {
            report_parse_error(parser_state, "expect expression in indexing statement");
            destroy_statement(indexing_stmt);
            return NULL;
        }
        add_child(&indexing_stmt->list, expr);
        
        if (!token_match(parser_state, 1, TOKEN_CLOSED_BRACKET)) {
            report_parse_error(parser_state, "expect ']' in indexing statement");
            destroy_statement(indexing_stmt);
            return NULL;
        }
        advance_token(parser_state);
        return indexing_stmt;
    }

    if (token_match(parser_state, 1, TOKEN_DOT)) {
        struct statement *member_stmt = make_statement(parser_state, MEMBER_STMT);
        if (!member_stmt) return NULL;
        advance_token(parser_state);
        struct statement *member = parse_basic(parser_state);
        
        if (!member) {
            report_parse_error(parser_state, "expect identifier after '.' operator");
            destroy_statement(member_stmt);
            return NULL;
        }
        add_child(&member_stmt->list, member);
        return member_stmt;
    }

    if (token_match(parser_state, 2, TOKEN_INCREMENT, TOKEN_DECREMENT)) {
        struct statement *stmt = make_statement(parser_state, POSTFIX_UNARY_STMT);
        if (!stmt) return NULL;
        advance_token(parser_state);
        return stmt;
    }
    
    return NULL; // For empty expressions
}

struct statement *parse_arg_list(struct parser_state * parser_state)
{
    struct statement *root = make_statement(parser_state, ARG_LIST);
    if (!root) return NULL;
    
    if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
        struct statement *first = parse_assignment(parser_state);
        
        if (!first) {
            destroy_statement(root);
            return NULL;
        }
        add_child(&root->list, first);

        while (token_match(parser_state, 1, TOKEN_COMMA)) {
            advance_token(parser_state);
            struct statement *next = parse_assignment(parser_state);
            
            if (!next) {
                report_parse_error(parser_state, "expect expression after ',' in argument list");
                destroy_statement(root);
                return NULL;
            }
            add_child(&root->list, next);
        }
    }
    return root;
}

struct statement *parse_basic(struct parser_state *parser_state)
{
    if (token_match(parser_state, 6, TOKEN_IDENTIFIER, TOKEN_STR_LITERAL, TOKEN_NUMBER, TOKEN_TRUE, TOKEN_FALSE, TOKEN_NIL)) {
        struct statement *stmt = make_statement(parser_state, BASIC_STMT);
        if (!stmt) return NULL;
        advance_token(parser_state);
        return stmt;
    }

    if (token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        advance_token(parser_state);
        struct statement *child = parse_expr(parser_state);
        
        if (!child) {
            report_parse_error(parser_state, "expect expression here in grouping expression");
            return NULL;
        }

        if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
            report_parse_error(parser_state, "expect ')' here in the end of grouping expression");
            destroy_statement(child);
            return NULL;
        }
        advance_token(parser_state);
        return child;
    }
    return NULL;
}

struct statement *parse_break(struct parser_state *parser_state) {
    struct statement *stmt = make_statement(parser_state, BREAK_STMT);
    if (!stmt) return NULL;
    advance_token(parser_state);
    
    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        report_parse_error(parser_state, "expect ';' after break");
        destroy_statement(stmt);
        return NULL;
    }
    advance_token(parser_state);
    return stmt;
}

struct statement *parse_continue(struct parser_state *parser_state) {
    struct statement *stmt = make_statement(parser_state, CONTINUE_STMT);
    if (!stmt) return NULL;
    advance_token(parser_state);
    
    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        report_parse_error(parser_state, "expect ';' after continue");
        destroy_statement(stmt);
        return NULL;
    }
    advance_token(parser_state);
    return stmt;
}

struct statement *parse_return(struct parser_state *parser_state) {
    struct statement *stmt = make_statement(parser_state, RETURN_STMT);
    if (!stmt) return NULL;
    advance_token(parser_state);
    
    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        struct statement *expr = parse_expr(parser_state);
        if (!expr) {
            report_parse_error(parser_state, "expect expression after return");
            destroy_statement(stmt);
            return NULL;
        }
        add_child(&stmt->list, expr);
        if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
            report_parse_error(parser_state, "expect ';' after return expression");
            destroy_statement(stmt);
            return NULL;
        }
    }
    advance_token(parser_state);
    return stmt;
}

struct statement *parse_print(struct parser_state *parser_state) {
    struct statement *stmt = make_statement(parser_state, PRINT_STMT);
    if (!stmt) return NULL;
    advance_token(parser_state);
    struct statement *expr = parse_expr(parser_state);
    if (!expr) {
        report_parse_error(parser_state, "expect expression after print");
        destroy_statement(stmt);
        return NULL;
    }
    add_child(&stmt->list, expr);
    
    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        report_parse_error(parser_state, "expect ';' after print statement");
        destroy_statement(stmt);
        return NULL;
    }
    advance_token(parser_state);
    return stmt;
}

struct statement *parse_bitwise_and(struct parser_state *parser_state) {
    struct statement *left = parse_equality(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 1, TOKEN_AND)) {
        struct statement *root = make_statement(parser_state, BITWISE_AND_STMT);
        if (!root) { destroy_statement(left); return NULL; }
        advance_token(parser_state);
        struct statement *right = parse_equality(parser_state);
        
        if (!right) {
            char error_buffer[40];
            sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
            report_parse_error(parser_state, error_buffer);
            destroy_statement(root);
            destroy_statement(left);
            return NULL;
        }
        add_child(&root->list, left);
        add_child(&root->list, right);
        left = root;
    }
    return left;
}

struct statement *parse_bitwise_xor(struct parser_state *parser_state) {
    struct statement *left = parse_bitwise_and(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 1, TOKEN_XOR)) {
        struct statement *root = make_statement(parser_state, BITWISE_XOR_STMT);
        if (!root) { destroy_statement(left); return NULL; }
        advance_token(parser_state);
        struct statement *right = parse_bitwise_and(parser_state);
        
        if (!right) {
            char error_buffer[40];
            sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
            report_parse_error(parser_state, error_buffer);
            destroy_statement(root);
            destroy_statement(left);
            return NULL;
        }
        add_child(&root->list, left);
        add_child(&root->list, right);
        left = root;
    }
    return left;
}

struct statement *parse_bitwise_or(struct parser_state *parser_state) {
    struct statement *left = parse_bitwise_xor(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 1, TOKEN_OR)) {
        struct statement *root = make_statement(parser_state, BITWISE_OR_STMT);
        if (!root) { destroy_statement(left); return NULL; }
        advance_token(parser_state);
        struct statement *right = parse_bitwise_xor(parser_state);
        
        if (!right) {
            char error_buffer[40];
            sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
            report_parse_error(parser_state, error_buffer);
            destroy_statement(root);
            destroy_statement(left);
            return NULL;
        }
        add_child(&root->list, left);
        add_child(&root->list, right);
        left = root;
    }
    return left;
}

struct statement *parse_logical_and(struct parser_state *parser_state) {
    struct statement *left = parse_bitwise_or(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 1, TOKEN_AND_AND)) {
        struct statement *root = make_statement(parser_state, LOGICAL_AND_STMT);
        if (!root) { destroy_statement(left); return NULL; }
        advance_token(parser_state);
        struct statement *right = parse_bitwise_or(parser_state);
        
        if (!right) {
            char error_buffer[40];
            sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
            report_parse_error(parser_state, error_buffer);
            destroy_statement(root);
            destroy_statement(left);
            return NULL;
        }
        add_child(&root->list, left);
        add_child(&root->list, right);
        left = root;
    }
    return left;
}

struct statement *parse_logical_or(struct parser_state *parser_state) {
    struct statement *left = parse_logical_and(parser_state);
    if (!left) return NULL;
    while (token_match(parser_state, 1, TOKEN_OR_OR)) {
        struct statement *root = make_statement(parser_state, LOGICAL_OR_STMT);
        if (!root) { destroy_statement(left); return NULL; }
        advance_token(parser_state);
        struct statement *right = parse_logical_and(parser_state);
        
        if (!right) {
            char error_buffer[40];
            sprintf(error_buffer, "Expect expression after %s", root->node.lexeme);
            report_parse_error(parser_state, error_buffer);
            destroy_statement(root);
            destroy_statement(left);
            return NULL;
        }
        add_child(&root->list, left);
        add_child(&root->list, right);
        left = root;
    }
    return left;
}

struct statement *parse_conditional(struct parser_state *parser_state) {
    struct statement *left = parse_logical_or(parser_state);
    if (!left) return NULL;
    if (token_match(parser_state, 1, TOKEN_QUESTION)) {
        struct statement *root = make_statement(parser_state, CONDITIONAL_STMT);
        if (!root) { destroy_statement(left); return NULL; }
        advance_token(parser_state);
        struct statement *middle = parse_expr(parser_state);
        
        if (!middle) {
            report_parse_error(parser_state, "expect expression after '?' in conditional expression");
            destroy_statement(root);
            destroy_statement(left);
            return NULL;
        }
        add_child(&root->list, left);
        add_child(&root->list, middle);
        
        if (!token_match(parser_state, 1, TOKEN_COLON)) {
            report_parse_error(parser_state, "expect ':' in conditional expression");
            destroy_statement(root);
            return NULL;
        }
        advance_token(parser_state);
        struct statement *right = parse_conditional(parser_state);
        
        if (!right) {
            destroy_statement(root);
            return NULL;
        }
        add_child(&root->list, right);
        return root;
    }
    return left;
}

// Synchronization functions
void synchronize(struct parser_state *parser_state)
{
    while (peek_token(parser_state) && !token_match(parser_state, 1, TOKEN_EOF)) {
        switch (peek_token(parser_state)->token_type) {
            case TOKEN_IF: case TOKEN_WHILE: case TOKEN_FOR: case TOKEN_OPEN_CURLY:
            case TOKEN_VAR: case TOKEN_FUN: case TOKEN_BREAK: case TOKEN_CONTINUE:
            case TOKEN_RETURN: case TOKEN_PRINT:
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
    while (peek_token(parser_state) && !token_match(parser_state, 1, TOKEN_EOF)) {
        switch (peek_token(parser_state)->token_type) {
            case TOKEN_IF: case TOKEN_WHILE: case TOKEN_FOR: case TOKEN_OPEN_CURLY: case TOKEN_VAR: 
            case TOKEN_CLOSED_CURLY: case TOKEN_FUN: case TOKEN_BREAK: case TOKEN_CONTINUE:
            case TOKEN_RETURN: case TOKEN_PRINT:
                return;
            case TOKEN_SEMICOLON:
                advance_token(parser_state);
                return;
            default:
                advance_token(parser_state);
        }
    }
}