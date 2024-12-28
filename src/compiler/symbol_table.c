//
// Created by dgood on 12/5/24.
//

#include "symbol_table.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "../datastructures/arraylist.h"
#include "../datastructures/hashmap.h"

symbol_table *symbol_table_init(void) {
    symbol_table *table = malloc(sizeof(*table));
    if (table == NULL)
        err(EXIT_FAILURE, "malloc failed");
    table->symbol_count = 0;
    table->store        = hashtable_create(string_hash_function, string_equals,
                                    free, symbol_free);
    table->outer        = nullptr;
    table->free_symbols = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, nullptr);
    return table;
}

symbol_table *enclosed_symbol_table_init(symbol_table *outer) {
    symbol_table *table = symbol_table_init();
    table->outer        = outer;
    return table;
}

symbol *symbol_define(symbol_table *table, const char *name) {
    const symbol_scope scope = table->outer == NULL ? GLOBAL : LOCAL;
    symbol *           s     = symbol_init(name, scope, table->symbol_count++);
    char *             n     = strdup(name);
    if (n == NULL)
        err(EXIT_FAILURE, "malloc failed");
    hashtable_set(table->store, n, s);
    return s;
}

symbol *symbol_define_function(symbol_table *table, char *name) {
    symbol *s = symbol_init(name, FUNCTION_SCOPE, 0);
    char *  n = strdup(name);
    if (n == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    hashtable_set(table->store, n, s);
    return s;
}

static symbol *symbol_define_free(symbol_table *table, symbol *original) {
    arraylist_add(table->free_symbols, original);
    symbol *sym = symbol_init(original->name, FREE,
                              table->free_symbols->size - 1);
    hashtable_set(table->store, strdup(original->name), sym);
    return sym;
}

symbol *symbol_define_builtin(const symbol_table *table, const size_t index,
                              const char *        name) {
    symbol *s = symbol_init(name, BUILTIN, index);
    char *  n = strdup(name);
    if (n == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    hashtable_set(table->store, n, s);
    return s;
}

symbol *symbol_init(const char *   name, const symbol_scope scope,
                    const uint16_t index) {
    symbol *s = malloc(sizeof(symbol));
    if (s == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    s->name = strdup(name);
    if (s->name == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    s->scope = scope;
    s->index = index;
    return s;
}

symbol *symbol_resolve(symbol_table *table, const char *name) {
    if (table->store == NULL)
        return NULL;
    void *obj = hashtable_get(table->store, (void *) name);
    if (obj == NULL && table->outer != NULL) {
        symbol *sym = symbol_resolve(table->outer, name);
        if (sym == NULL)
            return nullptr;
        if (sym->scope == GLOBAL || sym->scope == BUILTIN)
            return sym;
        return symbol_define_free(table, sym);
    }
    return obj;
}

void symbol_free(void *o) {
    free(((symbol *) o)->name);
    free(o);
}

void symbol_table_free(symbol_table *table) {
    hashtable_destroy(table->store);
    arraylist_destroy(table->free_symbols);
    free(table);
}
