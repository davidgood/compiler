//
// Created by dgood on 12/19/24.
//

#ifndef NODE_COMPILER_H
#define NODE_COMPILER_H
#include "compiler_core.h"

compiler_error compile_expression_node(compiler *compiler, ast_expression *expression_node);
compiler_error compile_statement_node(compiler *compiler, ast_statement *statement_node);

#endif //NODE_COMPILER_H
