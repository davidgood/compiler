//
// Created by dgood on 12/7/24.
//

#include "repl.h"
#include <err.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../ast/ast.h"
#include "../compiler/compiler_core.h"
#include "../evaluator/evaluator.h"
#include "../lexer/lexer.h"
#include "../object/builtins.h"
#include "../object/environment.h"
#include "../object/object.h"
#include "../parser/parser.h"
#include "../vm/virtual_machine.h"

static const char *PROMPT      = ">> ";
static const char *MONKEY_FACE = "            __,__\n\
   .--.  .-\"     \"-.  .--.\n\
  / .. \\/  .-. .-.  \\/ .. \\\n\
 | |  '|  /   Y   \\  |'  | |\n\
 | \\   \\  \\ 0 | 0 /  /   / |\n\
  \\ '- ,\\.-\"\"\"\"\"\"\"-./, -' /\n\
   ''-' /_   ^ ^   _\\ '-''\n\
       |  \\._   _./  |\n\
       \\   \\ '~' /   /\n\
        '._ '-=-' _.'\n\
           '-----'\n\
";

static void dump_bytecode(bytecode *bytecode) {
    object_compiled_fn *fn;
    object_int *        int_obj;
    char *              instructions = instructions_to_string(bytecode->instructions);
    printf(" Instructions:\n%s\n", instructions);
    free(instructions);
    if (bytecode->constants_pool == NULL)
        return;
    for (size_t i = 0; i < bytecode->constants_pool->size; i++) {
        object_object *constant = arraylist_get(bytecode->constants_pool, i);
        printf("CONSTANT %zu %p %s:\n", i, constant, get_type_name(constant->type));
        switch (constant->type) {
            case OBJECT_COMPILED_FUNCTION:
                fn = (object_compiled_fn *) constant;
                char *ins = instructions_to_string(fn->instructions);
                printf(" Instructions:\n%s", ins);
                free(ins);
                break;
            case OBJECT_INT:
                int_obj = (object_int *) constant;
                printf(" Value: %ld\n", int_obj->value);
                break;
            default:
                break;
        }
        printf("\n");
    }
}

static void print_parse_errors(const parser *parser) {
    printf("%s\n", MONKEY_FACE);
    printf("Woops! We ran into some monkey business here!\n");
    printf(" Parser errors:\n");
    list_node *list_node = parser->errors->head;
    while (list_node) {
        printf("\t%s\n", (char *) list_node->data);
        list_node = list_node->next;
    }
}


static void free_lines(arraylist *lines) {
    for (size_t i = 0; i < lines->capacity; i++) {
        free(lines->body[i]);
    }
    lines->capacity = 0;
}

int execute_file(const char *filename) {

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        switch (errno) {
            case EINVAL:
            case ENOMEM:
            case EACCES:
            case EINTR:
            case ELOOP:
            case EMFILE:
            case ENAMETOOLONG:
            case ENOENT:
            case EPERM:
            case EBADF:
                err(EXIT_FAILURE, "Failed to open file %s", filename);
                break;
            default:
                err(EXIT_FAILURE, "Failed to open file %s", filename);
        }
    }
    size_t       line_size      = 0;
    char *       line           = nullptr;
    char *       program_string = nullptr;
    environment *env            = environment_create();
    fseek(file, 0L, SEEK_END);
    const long file_size = ftell(file);
    rewind(file);
    program_string = malloc(file_size);

    if (program_string == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }

    while (getline(&line, &line_size, file) != -1) {
        strcat(program_string, line);
        line      = nullptr;
        line_size = 0;
    }

    fclose(file);

    lexer *      lexer    = lexer_init(program_string);
    parser *     parser   = parser_init(lexer);
    ast_program *program  = parse_program(parser);
    compiler *   compiler = compiler_init();

    printf("Executing file %s\n", program_string);

    free(program_string);

    if (parser->errors) {
        print_parse_errors(parser);
        goto EXIT;
    }

    compiler_error error = compile(compiler, (ast_node *) program);
    if (error.error_code != COMPILER_ERROR_NONE) {
        err(EXIT_FAILURE, "Failed to compile program");
    }
    bytecode *bytecode = get_bytecode(compiler);

    dump_bytecode(bytecode);

    virtual_machine *machine  = vm_init(bytecode);
    const vm_error   vm_error = vm_run(machine);

    if (vm_error.code != VM_ERROR_NONE) {
        err(EXIT_FAILURE, "Failed to run program");
    }
    object_object *object = vm_last_popped_stack_elem(machine);
    printf("Result: %s\n", object->inspect(object));

    //object_object *evaluated = evaluator_eval((ast_node *) program, env);
    environment_free(env);
    // if (evaluated != NULL) {
    //     if (evaluated->type != OBJECT_NULL) {
    //         char *s = evaluated->inspect(evaluated);
    //         printf("%s\n", s);
    //         free(s);
    //     }
    //     object_free(evaluated);
    // }

EXIT:
    program_free(program);
    parser_free(parser);
    if (line)
        free(line);
    return 0;
}

int repl(void) {
    ssize_t      bytes_read;
    size_t       line_size = 0;
    char *       line      = nullptr;
    parser *     parser    = nullptr;
    ast_program *program   = nullptr;
    environment *env       = environment_create();
    printf("%s\n", MONKEY_FACE);
    printf("Welcome to the monkey programming language\n");
    printf("%s", PROMPT);
    arraylist *lines = arraylist_create(4, free);
    while ((bytes_read = getline(&line, &line_size, stdin)) != -1) {
        if (strcmp(line, "quit\n") == 0)
            break;

        if (line[bytes_read - 2] == '\\') {
            line[bytes_read - 2] = 0;
            arraylist_add(lines, line);
            line      = nullptr;
            line_size = 0;
            printf("%s", "    ");
            continue;
        }
        arraylist_add(lines, line);
        line      = nullptr;
        line_size = 0;

        char * program_string = arraylist_zip(lines, "\n");
        lexer *l              = lexer_init(program_string);
        parser                = parser_init(l);
        program               = parse_program(parser);

        if (parser->errors) {
            print_parse_errors(parser);
            goto CONTINUE;
        }

        object_object *evaluated = evaluator_eval((ast_node *) program, env);
        if (evaluated != NULL) {
            char *s = evaluated->inspect(evaluated);
            printf("%s\n", s);
            free(s);
            object_free(evaluated);
        }

    CONTINUE:
        program_free(program);
        parser_free(parser);
        free_lines(lines);
        free(program_string);
        line    = nullptr;
        program = nullptr;
        parser  = nullptr;
        printf("%s", PROMPT);
    }

    if (program) {
        program_free(program);
    }
    if (parser) {
        parser_free(parser);
    }
    if (line) {
        free(line);
    }
    arraylist_destroy(lines);
    environment_free(env);
    return 0;
}




















