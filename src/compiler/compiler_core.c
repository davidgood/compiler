
#include <err.h>
#include <stdarg.h>
#include <stdlib.h>
#include "../object/object.h"
#include "../opcode/opcode.h"
#include "../object/builtins.h"
#include "node_compiler.h"
#include "compiler_core.h"
#include "compiler_utils.h"
#include "scope.h"

#define CONSTANTS_POOL_INIT_SIZE 16

/***************************************************************
********************** INIT FUNCTIONS **************************
 ***************************************************************/
compiler *compiler_init(void) {
    compiler *compiler = malloc(sizeof(*compiler));
    if (compiler == NULL) {
        err(EXIT_FAILURE, "Could not allocate memory for compiler");
    }
    compiler->constants_pool = NULL;
    compiler->symbol_table = symbol_table_init();
    for (size_t i = 0; i < get_builtins_count(); i++) {
        const char *builtin_name = (char *) get_builtins_name(i);
        if (builtin_name == NULL) {
            break;
        }
        symbol_define_builtin(compiler->symbol_table, i, builtin_name);
    }
    compiler->scope_index = 0;
    compiler->scopes = arraylist_create(16, _scope_free);
    compilation_scope *main_scope = scope_init();
    arraylist_add(compiler->scopes, main_scope);

    return compiler;
}

compiler *compiler_init_with_state(const symbol_table *symbol_table, arraylist *constants) {
    compiler *compiler = compiler_init();
    symbol_table_free(compiler->symbol_table);
    compiler->symbol_table = symbol_table_copy(symbol_table);
    compiler->constants_pool = arraylist_clone(constants, _copy_object);
    return compiler;
}

/***************************************************************
********************** HELPER FUNCTIONS ************************
 ***************************************************************/
size_t add_constant(compiler *compiler, object_object *obj) {
    if (compiler->constants_pool == NULL) {
        compiler->constants_pool = arraylist_create(CONSTANTS_POOL_INIT_SIZE, object_free);
    }
    arraylist_add(compiler->constants_pool, obj);
    return compiler->constants_pool->size - 1;
}

compiler_error compile(compiler *compiler, ast_node *node) {
    compiler_error error;
    compiler_error none_error = {COMPILER_ERROR_NONE, NULL};
    switch (node->type) {
    case PROGRAM:
        ast_program *program = (ast_program *) node;
        for (size_t i = 0; i < program->statement_count; i++) {
            error = compile(compiler, (ast_node *) program->statements[i]);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
        }
        break;
    case STATEMENT:
        ast_statement *statement_node = (ast_statement *) node;
        error = compile_statement_node(compiler, statement_node);
        if (error.error_code != COMPILER_ERROR_NONE)
            return error;
        break;
    case EXPRESSION:
        ast_expression *expression_node = (ast_expression *) node;
        error = compile_expression_node(compiler, expression_node);
        if (error.error_code != COMPILER_ERROR_NONE)
            return error;
        break;
    default:
        return none_error;
    }
    return none_error;
}

instructions *compiler_leave_scope(compiler *compiler) {
    const compilation_scope *scope = get_top_scope(compiler);
    instructions *ins = opcode_copy_instructions(scope->instructions);
    arraylist_remove(compiler->scopes, compiler->scope_index);
    compiler->scope_index--;
    symbol_table *table = compiler->symbol_table;
    compiler->symbol_table = compiler->symbol_table->outer;
    symbol_table_free(table);
    return ins;
}

void compiler_enter_scope(compiler *compiler) {
    compilation_scope *scope = scope_init();
    arraylist_add(compiler->scopes, scope);
    compiler->scope_index++;
    compiler->symbol_table = enclosed_symbol_table_init(compiler->symbol_table);
}

/***************************************************************
********************** FREE FUNCTIONS **************************
 ***************************************************************/

void compiler_free(compiler *compiler) {
    arraylist_destroy(compiler->scopes);
    if (compiler->constants_pool) {
        arraylist_destroy(compiler->constants_pool);
    }
    symbol_table_free(compiler->symbol_table);
    free(compiler);
}

