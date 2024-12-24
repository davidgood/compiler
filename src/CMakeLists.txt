cmake_minimum_required(VERSION 3.30)
project(compiler C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(compiler main.c
        token/token.c
        token/token.h
        datastructures/hashmap.c
        datastructures/hashmap.h
        lexer/lexer.c
        lexer/lexer.h
        ast/ast.h
        ast/ast_debug_print.h
        ast/ast_debug_print.c
        datastructures/arraylist.c
        datastructures/arraylist.h
        object/environment.c
        object/environment.h
        object/object.c
        object/object.h
        opcode/opcode.c
        opcode/opcode.h
        parser/parser.c
        parser/parser.h
        parser/parser_tracing.c
        parser/parser_tracing.h
        datastructures/stack.c
        datastructures/stack.h
        compiler/symbol_table.c
        compiler/symbol_table.h
        evaluator/evaluator.c
        evaluator/evaluator.h
        object/builtins.c
        object/builtins.h
        datastructures/linked_list.c
        datastructures/linked_list.h
        datastructures/conversions.c
        datastructures/conversions.h
        repl/repl.c
        repl/repl.h
        vm/virtual_machine.c
        vm/virtual_machine.h
        vm/frame.c
        vm/frame.h
        compiler/instructions.c
        compiler/instructions.h
        compiler/scope.c
        compiler/scope.h
        compiler/node_compiler.c
        compiler/node_compiler.h
        compiler/compiler_utils.c
        compiler/compiler_utils.h
        compiler/compiler_core.c
        compiler/compiler_core.h
        logging/log.c
        logging/log.h)