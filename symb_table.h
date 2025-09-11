#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "uthash.h"
#include "parser.h"
#include "lexer.h"
#include <stdbool.h>

struct symbol_table_entry {
    char *name;
    struct literal *value;
    struct statement *decl;
    UT_hash_handle hh;
};

extern struct symbol_table_entry *variable_symbols;
extern struct symbol_table_entry *function_symbols;

void add_symbol(struct symbol_table_entry **table, char *name, struct literal *value, struct statement *decl);
struct symbol_table_entry *find_symbol(struct symbol_table_entry **table, char *name);
void delete_symbol(struct symbol_table_entry **table, char *name);
void clear_symbol_table(struct symbol_table_entry **table);

#endif