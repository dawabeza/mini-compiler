#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

//helper function for visualization of the tree in graphviz

void dump_statement_dot(FILE *out, struct statement *stmt, int *id, int parent_id) {
    if (!stmt) return;

    int my_id = (*id)++;
    char *node = node_name(stmt);
    fprintf(out, "  node%d [label=\"%s\"];\n", my_id, node);
    free(node);
    if (parent_id >= 0) {
        fprintf(out, "  node%d -> node%d;\n", parent_id, my_id);
    }

    for (int i = 0; i < stmt->list.count; i++) {
        dump_statement_dot(out, stmt->list.data[i], id, my_id);
    }
}

void dump_ast_dot(struct statement *root, const char *filename) {
    FILE *out = fopen(filename, "w");
    if (!out) return;

    fprintf(out, "digraph AST {\n");
    int id = 0;
    dump_statement_dot(out, root, &id, -1);
    fprintf(out, "}\n");

    fclose(out);
}


int main() {
    //lexing
    struct lexer_state lexer_state;
    init_lexer_state(&lexer_state);
    scan(&lexer_state);
    if (lexer_state.had_error) {
        destroy_lexer_state(&lexer_state);
        printf("EXIT WITH LEXER ERROR"); exit(1);
    }
    //the parsing stage
    struct parser_state parser_state;
    init_parser_state(&parser_state, &lexer_state);
    struct statement *syntax_tree = parse_program(&parser_state);
    if (parser_state.had_error) {
        destroy_statement(syntax_tree);
        printf("EXIT WITH ABOVE ERRORS");
        exit(1);
    }
    
    dump_ast_dot(syntax_tree, "myast.dot");
    destroy_statement(syntax_tree);
    destroy_lexer_state(&lexer_state);

}