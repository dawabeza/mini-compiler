#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    //lexing
    struct lexer_state lexer_state;
    init_lexer_state(&lexer_state);
    scan(&lexer_state);

    //the parsing stage
    struct parser_state parser_state;
    init_parser_state(&parser_state, &lexer_state);
    struct expr *syntax_tree = parse_full_expr(&parser_state);
    pretty_printer(syntax_tree, 0);
    destroy_expr(syntax_tree);
    destroy_lexer_state(&lexer_state);

}