#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    struct lexer_state *lexer_state = malloc(sizeof(struct lexer_state));
    init_lexer_state(lexer_state);
    scan(lexer_state);
    print_tokens(lexer_state);
    destroy_lexer_state(lexer_state);
}