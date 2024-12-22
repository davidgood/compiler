//
// Created by dgood on 12/19/24.
//

#include "scope.h"
#include "compiler_utils.h"
#include "../opcode/opcode.h"
#include <err.h>

compilation_scope *scope_init() {
    compilation_scope *scope = malloc(sizeof(compilation_scope));
    if (scope == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    scope->instructions = malloc(sizeof(instructions));
    if (scope->instructions == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    scope->instructions->bytes    = NULL;
    scope->instructions->length   = 0;
    scope->instructions->capacity = 0;
    return scope;
}

void _scope_free(void *scope) {
    scope_free(scope);
}

void scope_free(compilation_scope *scope) {
    if (!scope) {
        return;
    }
    if (scope->instructions) {
        instructions_free(scope->instructions);
        scope->instructions = NULL;
    }
    free(scope);
}

compilation_scope *get_top_scope(const compiler *compiler) {
    return arraylist_get(compiler->scopes, compiler->scope_index);
}

symbol_table *symbol_table_copy(const symbol_table *src) {
    symbol_table *new_table = symbol_table_init();
    hashtable_destroy(new_table->store);
    new_table->store        = hashtable_clone(src->store, _strdup, _copy_symbol);
    new_table->symbol_count = src->symbol_count;
    return new_table;
}
