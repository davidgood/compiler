//
// Created by dgood on 12/5/24.
//

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdint.h>
#include "../datastructures/arraylist.h"
#include "../datastructures/hashmap.h"

typedef enum symbol_scope {
    GLOBAL,
    LOCAL,
    BUILTIN,
    FREE,
    FUNCTION_SCOPE
} symbol_scope;

static char *scope_names[] = {
        "GLOBAL",
        "LOCAL",
        "BUILTIN",
        "FREE",
        "FUNCTION"
};

#define get_scope_name(s) scope_names[s]

typedef struct {
    char *         name;
    symbol_scope scope;
    uint16_t       index;
} symbol;

typedef struct symbol_table {
    struct symbol_table *outer;
    hashtable *          store;
    arraylist *          free_symbols;
    uint16_t             symbol_count;
} symbol_table;

symbol_table *symbol_table_init(void);

symbol_table *enclosed_symbol_table_init(symbol_table *);

symbol *symbol_define(symbol_table *, const char *);

symbol *symbol_define_builtin(const symbol_table *table, const size_t index,
                              const char *        name);

symbol *symbol_define_function(symbol_table *, char *);

symbol *symbol_resolve(symbol_table *, const char *);

void symbol_table_free(symbol_table *);

void symbol_free(void *);

symbol *symbol_init(const char *, symbol_scope, uint16_t);
#endif //SYMBOL_TABLE_H
