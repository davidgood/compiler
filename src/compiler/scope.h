//
// Created by dgood on 12/19/24.
//

#ifndef SCOPE_H
#define SCOPE_H
#include "compiler_core.h"

compilation_scope *scope_init(void);

void _scope_free(void *scope);
void scope_free(compilation_scope *);

compilation_scope *get_top_scope(const compiler *);
symbol_table *symbol_table_copy(const symbol_table *);
#endif //SCOPE_H
