#include "symb_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct symbol_table_entry *variable_symbols = NULL;
struct symbol_table_entry *function_symbols = NULL;

void add_symbol(struct symbol_table_entry **table, char *name, struct literal *value, struct statement *decl) {
    struct symbol_table_entry *entry;

    HASH_FIND_STR(*table, name, entry);

    if (entry == NULL) {
        entry = (struct symbol_table_entry *)malloc(sizeof(struct symbol_table_entry));
        if (entry == NULL) {
            fprintf(stderr, "Memory allocation failed for symbol table entry.\n");
            exit(1);
        }

        entry->name = strdup(name);
        entry->value = value;
        entry->decl = decl;

        HASH_ADD_STR(*table, name, entry);
    } else {
        fprintf(stderr, "Error: Re-declaration of symbol '%s'\n", name);
    }
}

struct symbol_table_entry *find_symbol(struct symbol_table_entry **table, char *name) {
    struct symbol_table_entry *entry;
    HASH_FIND_STR(*table, name, entry);
    return entry;
}

void delete_symbol(struct symbol_table_entry **table, char *name) {
    struct symbol_table_entry *entry;
    HASH_FIND_STR(*table, name, entry);

    if (entry) {
        HASH_DEL(*table, entry);
        free(entry->name);
        free(entry);
    }
}

void clear_symbol_table(struct symbol_table_entry **table) {
    struct symbol_table_entry *current_entry, *tmp;

    HASH_ITER(hh, *table, current_entry, tmp) {
        HASH_DEL(*table, current_entry);
        free(current_entry->name);
        free(current_entry);
    }

    *table = NULL;
}