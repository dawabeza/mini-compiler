#include "lexer.h"
#include "parser.h"
#include "error_handling.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

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


void destroy_statement(struct statement *stmts)
{
    for (int i = 0; i < stmts->list.count; i++) {
        destroy_statement(&stmts->list.data[i]);
    }
    free(stmts->list.data);
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

bool token_match(struct parser_state *parser_state, int count,  ...)
{
    va_list args;
    va_start(args, count);
    enum token_type type;
    struct token *cur = peek_token(parser_state);
    for (int i = 0; i < count; i++) {
        type = va_arg(args, enum token_type);
        if (type == cur->token_type) {
            return true;
        }
    }
        
    return false;
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

void print_node(struct statement *stmt) {
    switch (stmt->type) {
        // Directly printable types
        case BASIC_STMT:
        case TERM_STMT:
        case FACTOR_STMT:
        case UNARY_STMT:
        case ASSIGNMENT_STMT:
        case EQUALITY_STMT:
        case COMPARISON_STMT:
        case FUN_DECL_STMT:
        case VAR_DECL_STMT:
        case COMMA_SEPARATED_STMT:
        case PARAM_LIST:
        case ARG_LIST:
            printf("%s\n", stmt->node.lexeme);
            break;
        // Structural/abstract types
        case BLOCK_STMT: printf("BLOCK\n"); break;
        case IF_STMT: printf("IF\n"); break;
        case FOR_STMT: printf("FOR\n"); break;
        case WHILE_STMT: printf("WHILE\n"); break;
        case ERROR_EXPR: printf("ERROR\n"); break;
        case EMPTY_EXPR: printf("EMPTY\n"); break;
        default:
            printf("UNKNOWN\n");
            break;
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
    struct statement full_program = make_statement(parser_state, BLOCK_STMT);
    while (!token_match(parser_state, 1, TOKEN_EOF)) {
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
            return make_statement(parser_state, ERROR_EXPR);
        default:
            return parse_expr_stmt(parser_state);
    }
}

struct statement parse_block(struct parser_state *parser_state)
{
    struct statement block = make_statement(parser_state, BLOCK_STMT);
    advance_token(parser_state);
    while (!token_match(parser_state, 1, TOKEN_EOF) && peek_token(parser_state)->token_type != TOKEN_CLOSED_CURLY) {
        struct statement stmt = parse_statement(parser_state);
        if (stmt.type == ERROR_EXPR) {
            synchronize_block(parser_state);
        }
        add_child(&block.list, &stmt);
    }


    if (!token_match(parser_state, 1, TOKEN_CLOSED_CURLY)) {
        report_parse_error(parser_state, "expect '} after block statement");
        block.type = ERROR_EXPR;
        return block;
    }
    advance_token(parser_state);
    return block;
}


struct statement parse_fun_decl(struct parser_state *parser_state)
{
    struct statement fun_decl = make_statement(parser_state, FUN_DECL_STMT);
    advance_token(parser_state); // consume "fun"
    // Expect IDENTIFIER
    if (!token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
        report_parse_error(parser_state, "Expected function name after 'fun'");
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }
    struct statement name = make_statement(parser_state, BASIC_STMT);
    advance_token(parser_state);
    add_child(&fun_decl.list, &name);

    // Expect "("
    if (!token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        report_parse_error(parser_state, "Expected '(' after function name");
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }
    advance_token(parser_state); // consume "("

    if (token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
        struct statement params = parse_param_list(parser_state);
        add_child(&fun_decl.list, &params);
        if (params.type == ERROR_EXPR) {
            fun_decl.type = ERROR_EXPR;
            return fun_decl;
        }
    }
    // Expect ")"
    if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
        report_parse_error(parser_state, "Expected ')' after parameter list");
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }
    advance_token(parser_state); // consume ")"

    // Expect BLOCK_STMT
    if (!token_match(parser_state, 1, TOKEN_OPEN_CURLY)) {
        report_parse_error(parser_state, "Expected '{' to start function body");
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }
    struct statement body = parse_block(parser_state);
    add_child(&fun_decl.list, &body);
    if (body.type == ERROR_EXPR) {
        add_child(&fun_decl.list, &body);
        fun_decl.type = ERROR_EXPR;
        return fun_decl;
    }


    return fun_decl;
}

struct statement parse_param_list(struct parser_state *parser_state)
{
    struct statement param_list = make_statement(parser_state, PARAM_LIST);

    if (!token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
        report_parse_error(parser_state, "Expected parameter identifier");
        param_list.type = ERROR_EXPR;
        return param_list;
    }
    // Parse first IDENTIFIER
    struct statement param = parse_basic(parser_state);
    add_child(&param_list.list, &param);

    // Parse (',' IDENTIFIER)*
    while (token_match(parser_state, 1, TOKEN_COMMA)) {
        advance_token(parser_state); // consume ','
        if (!token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
            report_parse_error(parser_state, "Expected identifier after ',' in parameter list");
            param_list.type = ERROR_EXPR;
            return param_list;
        }
        struct statement next_param = parse_basic(parser_state);
        add_child(&param_list.list, &next_param);
    }
    return param_list;
}

struct statement parse_var_decl(struct parser_state *parser_state)
{
    struct statement decl_stmt = make_statement(parser_state, VAR_DECL_STMT);
    advance_token(parser_state);
    if (token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
        struct statement identifier = parse_basic(parser_state);
        add_child(&decl_stmt.list, &identifier);
    }
    else {
        report_parse_error(parser_state, "expect identifier after variable declarator 'var'");
        decl_stmt.type = ERROR_EXPR;
        return decl_stmt;
    }
    //optional initializer for variable declaration
    if (token_match(parser_state, 1, TOKEN_ASSIGNMENT)) {
        advance_token(parser_state);
        struct statement initializer = parse_expr(parser_state);
        add_child(&decl_stmt.list, &initializer);
        if (initializer.type == ERROR_EXPR) {
            decl_stmt.type = ERROR_EXPR;
            return decl_stmt;         
        }
    }

    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        report_parse_error(parser_state, "expect ; after variable declaration");
        decl_stmt.type = ERROR_EXPR;
        return decl_stmt;
    }
    advance_token(parser_state);

    return decl_stmt;
}

struct statement parse_if(struct parser_state *parser_state)
{
    struct statement if_stmt = make_statement(parser_state, IF_STMT);
    advance_token(parser_state);
     if (!token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        report_parse_error(parser_state, "expect '(' after 'if' keyword");
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }
    advance_token(parser_state);
    struct statement condition_expr = parse_expr(parser_state);
    add_child(&if_stmt.list, &condition_expr);
    if (condition_expr.type == ERROR_EXPR) {
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }
    if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
        report_parse_error(parser_state, "expect ')' after if expression");
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }
    advance_token(parser_state);
    struct statement stmt = parse_statement(parser_state);
    add_child(&if_stmt.list, &stmt);

    if (stmt.type == ERROR_EXPR) {
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }

    if (stmt.type == EMPTY_EXPR) {
        if_stmt.type = ERROR_EXPR;
        report_parse_error(parser_state, "if expression cannot be empty");
        return if_stmt;
    }
    
   if (token_match(parser_state, 1, TOKEN_ELSE)) {
    advance_token(parser_state);
    struct statement else_stmt = parse_statement(parser_state);
    add_child(&if_stmt.list, &else_stmt);
    if (else_stmt.type == ERROR_EXPR) {
        if_stmt.type = ERROR_EXPR;
        return if_stmt;
    }
   }
    return if_stmt;
}

struct statement parse_for(struct parser_state *parser_state)
{
    struct statement for_stmt = make_statement(parser_state, FOR_STMT);
    advance_token(parser_state);
    if (!token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        report_parse_error(parser_state, "expect '(' after 'for' keyword");
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }
    advance_token(parser_state);
    struct statement init_expr = parse_expr_stmt(parser_state);
    add_child(&for_stmt.list, &init_expr);
    if (init_expr.type == ERROR_EXPR) {
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }
    struct statement test_expr = parse_expr_stmt(parser_state);
    add_child(&for_stmt.list, &test_expr);
    if (test_expr.type == ERROR_EXPR) {
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }

    struct statement increment_expr = parse_expr(parser_state);
    add_child(&for_stmt.list, &increment_expr);
    if (increment_expr.type == ERROR_EXPR) {
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }
     if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
        report_parse_error(parser_state, "expect ')' after for statement expression");
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }
    advance_token(parser_state);
    struct statement stmt = parse_statement(parser_state);
    add_child(&for_stmt.list, &stmt);
    if (stmt.type == ERROR_EXPR) {
        for_stmt.type = ERROR_EXPR;
        return for_stmt;
    }


    return for_stmt;
}

struct statement parse_while(struct parser_state *parser_state)
{
    struct statement while_stmt = make_statement(parser_state, WHILE_STMT);
    advance_token(parser_state);
    if (!token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        report_parse_error(parser_state, "expect '(' after while keyword");
       while_stmt.type = ERROR_EXPR;
       return while_stmt;
    }
    advance_token(parser_state);
    struct statement child_expr = parse_expr(parser_state);
    add_child(&while_stmt.list, &child_expr);
    if (child_expr.type == ERROR_EXPR) {
        while_stmt.type = ERROR_EXPR;
        return while_stmt;
    }
    if (child_expr.type == EMPTY_EXPR) {
        report_parse_error(parser_state, "expect expression here");
        while_stmt.type == ERROR_EXPR;
        return while_stmt;
    }
    if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
        report_parse_error(parser_state, "expect ')' after while experssion");
        while_stmt.type = ERROR_EXPR;
        return while_stmt;
    }
    advance_token(parser_state);
    struct statement child_stmt = parse_statement(parser_state);
    add_child(&while_stmt.list, &child_stmt);
    if (child_stmt.type == ERROR_EXPR) {
       while_stmt.type = ERROR_EXPR;
       return while_stmt; 
    }
    return while_stmt;
}

struct statement parse_expr_stmt(struct parser_state *parser_state)
{
    struct statement expr_stmt = parse_expr(parser_state);
    if (expr_stmt.type == ERROR_EXPR) {
        return expr_stmt;
    }
    if (!token_match(parser_state, 1, TOKEN_SEMICOLON)) {
        report_parse_error(parser_state, "expect ';' after end of the expression");
        expr_stmt.type = ERROR_EXPR;
        return expr_stmt;
    }
    advance_token(parser_state);
    return expr_stmt;
}

struct statement parse_expr(struct parser_state *parser_state)
{
    struct statement left = parse_assignment(parser_state);
    if (left.type == ERROR_EXPR) return left;
    while (token_match(parser_state, 1, TOKEN_COMMA)) {
        struct statement root = make_statement(parser_state, EQUALITY_STMT);
        advance_token(parser_state);
        struct statement right = parse_assignment(parser_state);
        add_child(&root.list, &left);
        add_child(&root.list, &right);
           if (right.type == ERROR_EXPR) {
                root.type = ERROR_EXPR;
                return right;
            }
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                root.type = ERROR_EXPR;
                return root;
            }
            left = root;
    }

    return left;
    
}

struct statement parse_assignment(struct parser_state *parser_state)
{
    struct statement left = parse_equality(parser_state);
    if (left.type == ERROR_EXPR) return left;
    if (token_match(parser_state, 5, TOKEN_ASSIGNMENT, TOKEN_PLUS_ASSIGN, TOKEN_MINUS_ASSIGN,
                                     TOKEN_STAR_ASSIGN, TOKEN_SLASH_ASSIGN, TOKEN_MODULO_ASSIGN)) {
            struct statement root = make_statement(parser_state, ASSIGNMENT_STMT);
            advance_token(parser_state);
            struct statement right = parse_assignment(parser_state);
            add_child(&root.list, &left);
            add_child(&root.list, &right);
            if (right.type == ERROR_EXPR) {
                root.type = ERROR_EXPR;
                return right;
            }
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                root.type = ERROR_EXPR;
                return root;
            }
            left = root;
    }

    return left;
}

struct statement parse_equality(struct parser_state * parser_state)
{
    struct statement left = parse_comparison(parser_state);
    if (left.type == ERROR_EXPR) return  left;
    while (token_match(parser_state, 2, TOKEN_EQUAL, TOKEN_BANG_EQUAL)) {
            struct statement root = make_statement(parser_state, EQUALITY_STMT);
            advance_token(parser_state);
            struct statement right = parse_comparison(parser_state);
            add_child(&root.list, &left);
            add_child(&root.list, &right);
            if (right.type == ERROR_EXPR) {
                root.type = ERROR_EXPR;
                return right;
            }
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                root.type = ERROR_EXPR;
                return root;
            }
            left = root;
    }

    return left;
    
}

struct statement parse_comparison(struct parser_state *parser_state)
{
    struct statement left = parse_term(parser_state);
    if (left.type == ERROR_EXPR) return left;
    while (token_match(parser_state, 4, TOKEN_GREATER_THAN, TOKEN_GREATER_EQUAL, TOKEN_LESS_THAN, TOKEN_LESS_EQUAL)) {
            struct statement root = make_statement(parser_state, COMPARISON_STMT);
            advance_token(parser_state);
            struct statement right = parse_term(parser_state);
            add_child(&root.list, &left);
            add_child(&root.list, &right);
           if (right.type == ERROR_EXPR) {
                root.type = ERROR_EXPR;
                return right;
            }
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                root.type = ERROR_EXPR;
                return root;
            }
            left = root;
    }

    return left;
}

struct statement parse_term(struct parser_state *parser_state)
{
    struct statement left = parse_factor(parser_state);
    if (left.type == ERROR_EXPR) return left;
    while (token_match(parser_state, 2, TOKEN_PLUS, TOKEN_MINUS)) {
            struct statement root = make_statement(parser_state, TERM_STMT);
            advance_token(parser_state);
            struct statement right = parse_factor(parser_state);
            add_child(&root.list, &left);
            add_child(&root.list, &right);
             if (right.type == ERROR_EXPR) {
                root.type = ERROR_EXPR;
                return root;
            }
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                root.type = ERROR_EXPR;
                return root;
            }
            left = root;
    }

    return left;

}

struct statement parse_factor(struct parser_state* parser_state)
{
    struct statement left = parse_unary(parser_state);
    if (left.type == ERROR_EXPR) return left;
    while (token_match(parser_state, 3, TOKEN_SLASH, TOKEN_STAR, TOKEN_MODULO)) {
            struct statement root = make_statement(parser_state, FACTOR_STMT);
            advance_token(parser_state);
            struct statement right = parse_unary(parser_state);
            add_child(&root.list, &left);
            add_child(&root.list, &right);
             if (right.type == ERROR_EXPR) {
                root.type = ERROR_EXPR;
                return right;
            }
            if (right.type == EMPTY_EXPR) {
                char error_buffer[40];
                sprintf(error_buffer, "the right operand of %s is missing", root.node.lexeme);
                report_parse_error(parser_state, error_buffer);
                root.type = ERROR_EXPR;
                return root;
            }
            left = root;
    }

    return left;
}

struct statement parse_unary(struct parser_state *parser_state)
{
    if (token_match(parser_state, 2, TOKEN_PLUS, TOKEN_MINUS)) {
        struct statement stmt = make_statement(parser_state, UNARY_STMT);
        advance_token(parser_state);
        struct statement child = parse_basic(parser_state);
        add_child(&stmt.list, &child);
        if (child.type == ERROR_EXPR) {
            stmt.type = ERROR_EXPR;
            return stmt;
        };
        if (child.type == EMPTY_EXPR) {
            char error_buffer[40];
            sprintf(error_buffer, "the right operand of %s is missing", stmt.node.lexeme);
            report_parse_error(parser_state, error_buffer);
            stmt.type = ERROR_EXPR;
            return stmt;
        }
        return stmt;
    }

    return parse_postfix(parser_state);
}

struct statement parse_postfix(struct parser_state * parser_state)
{
    struct statement root = parse_basic(parser_state);
    while (token_match(parser_state, TOKEN_OPEN_PARENTHESIS, TOKEN_OPEN_BRACKET, TOKEN_DOT)) {
       struct statement new_root = make_statement(parser_state, POSTFIX);
       struct statement operand = parse_postfix_tail(parser_state);
       add_child(&new_root.list, &root);
       add_child(&new_root.list, &operand);
        if (operand.type == ERROR_EXPR) {
            new_root.type = ERROR_EXPR;
            return new_root;
        }
        root = new_root;
    }
    return root;
}

struct statement parse_postfix_tail(struct parser_state * parser_state)
{
    if (token_match(parser_state, 1, TOKEN_OPEN_PARENTHESIS)) {
        advance_token(parser_state);
        struct statement arg_list = parse_arg_list(parser_state);
        if (arg_list.type == ERROR_EXPR) {
            arg_list.type =  ERROR_EXPR;
            return  arg_list;
        }
        if (arg_list.type == EMPTY_EXPR) {
            arg_list.type = EMPTY_EXPR;
            return arg_list;
        }
        if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
            report_parse_error(parser_state, "expect ')' after function call");
            arg_list.type = ERROR_EXPR;
            return  arg_list;
        }
        return arg_list;
    }

    if (token_match(parser_state,1, TOKEN_OPEN_BRACKET)) {
        advance_token(parser_state);
        struct statement expr = parse_expr(parser_state);
        if (expr.type == ERROR_EXPR) {
            expr.type = ERROR_EXPR;
            return expr;
        }
        if (expr.type == EMPTY_EXPR) {
            expr.type = EMPTY_EXPR;
            return expr;
        }
        if (!token_match(parser_state, 1, TOKEN_CLOSED_BRACKET)) {
            report_parse_error(parser_state, "expect ']'  in this indexing statement");
            expr.type = ERROR_EXPR;
            return expr;
        }
        return expr;
    }

    if (token_match(parser_state, 1, TOKEN_DOT)) {
        advance_token(parser_state);
        if (!token_match(parser_state, 1, TOKEN_IDENTIFIER)) {
            report_parse_error(parser_state, "expect identifier after using '.' operator");
            return make_statement(parser_state, ERROR_EXPR);
        }
        return parse_basic(parser_state);
    }

    return make_statement(parser_state, ERROR_EXPR);

}

struct statement parse_arg_list(struct parser_state * parser_state)
{  
    struct statement root = make_statement(parser_state, ARG_LIST);
    struct statement start = parse_assignment(parser_state);
    add_child(&root.list, &start);
    if (start.type == ERROR_EXPR) {
        root.type == ERROR_EXPR;
        return root;
    }
    if (start.type == EMPTY_EXPR) {
        root.type == ERROR_EXPR;
        return root;
    }
    while (token_match(parser_state, 1, TOKEN_COMMA)) {
            advance_token(parser_state);
            struct statement right = parse_assignment(parser_state);
            add_child(&root.list, &right);

            if (right.type == ERROR_EXPR) {
                root.type == ERROR_EXPR;
                return root;
            }
            if (right.type == EMPTY_EXPR) {
                report_parse_error(parser_state, "here we expect an expression after ','");
                root.type == ERROR_EXPR;
                return root;
            }
    }

    return root;
}

struct statement parse_basic(struct parser_state *parser_state)
{
    struct token *cur_token = peek_token(parser_state);

    if (cur_token->token_type == TOKEN_NUMBER || cur_token->token_type == TOKEN_STR_LITERAL || 
        cur_token->token_type == TOKEN_IDENTIFIER || cur_token->token_type == TOKEN_TRUE ||
        cur_token->token_type == TOKEN_FALSE || cur_token->token_type == TOKEN_NIL) {
        struct statement  stmt = make_statement(parser_state, BASIC_STMT);
        advance_token(parser_state);
        return stmt;
    }

    if (cur_token->token_type == TOKEN_OPEN_PARENTHESIS) {
        advance_token(parser_state);
        struct statement child = parse_expr(parser_state);
        if (child.type == ERROR_EXPR) return child;
        if (!token_match(parser_state, 1, TOKEN_CLOSED_PARENTHESIS)) {
            report_parse_error(parser_state, "expect ')' here in the end of grouping expression");
            child.type = ERROR_EXPR;
            return child;
        }
        advance_token(parser_state);
        return child;
    }

    return make_statement(parser_state, EMPTY_EXPR);
}

void synchronize(struct parser_state *parser_state)
{
    while (!token_match(parser_state, 1, TOKEN_EOF)) {
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
    while (!token_match(parser_state, 1, TOKEN_EOF)) {
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
