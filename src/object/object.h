//
// Created by dgood on 12/4/24.
//

#ifndef OBJECT_H
#define OBJECT_H

#include "../ast/ast.h"
#include "../datastructures/arraylist.h"
#include "environment.h"
#include "../opcode/opcode.h"
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    OBJECT_INT,
    OBJECT_BOOL,
    OBJECT_NULL,
    OBJECT_RETURN_VALUE,
    OBJECT_ERROR,
    OBJECT_FUNCTION,
    OBJECT_STRING,
    OBJECT_BUILTIN,
    OBJECT_ARRAY,
    OBJECT_HASH,
    OBJECT_COMPILED_FUNCTION,
    OBJECT_CLOSURE
} object_type;

static const char *type_names[] = {
    "INTEGER",
    "BOOLEAN",
    "NULL",
    "RETURN_VALUE",
    "MONKEY_ERROR",
    "FUNCTION",
    "STRING",
    "BUILTIN",
    "ARRAY",
    "HASH",
    "COMPILED_FUNCTION",
    "CLOSURE"
};

#define MAX_FREE_VARIABLES 256
#define get_type_name(type) type_names[type]

typedef struct object_object {
    object_type type;

    char *(*inspect)(struct object_object *);

    size_t (*hash)(void *);

    bool (*equals)(void *, void *);

    size_t refcount;
} object_object;

typedef struct {
    object_object object;
    long          value;
} object_int;

typedef struct {
    object_object object;
    bool          value;
} object_bool;

typedef struct {
    object_object object;
} object_null;

typedef struct {
    object_object  obj;
    object_object *value;
} object_return_value;

typedef struct {
    object_object object;
    char *        message;
} object_error;

typedef struct {
    object_object        object;
    linked_list *        parameters; // list of identifiers
    ast_block_statement *body;
    environment *        env;
} object_function;

typedef struct {
    object_object object;
    char *        value;
    size_t        length;
} object_string;

typedef struct {
    object_object object;
    instructions *instructions;
    size_t        num_locals;
    size_t        num_args;
} object_compiled_fn;

typedef object_object *(*builtin_fn)(linked_list *);

typedef struct {
    object_object object;
    builtin_fn    function;
} object_builtin;

typedef struct {
    object_object object;
    arraylist *   elements;
} object_array;

typedef struct {
    object_object object;
    hashtable *   pairs;
} object_hash;

typedef struct {
    object_object       object;
    object_compiled_fn *fn;
    object_object *     free_variables[MAX_FREE_VARIABLES];
    size_t              free_variables_count;
} object_closure;

char *inspect(object_object *);

bool object_equals(void *, void *);

size_t object_get_hash(void *); // non-static for tests

extern const object_bool TRUE_OBJ;
extern const object_bool FALSE_OBJ;
extern const object_null NULL_OBJ;

#define object_create_bool(val) ((val == true) ? ((object_bool *) &TRUE_OBJ) : ((object_bool *) &FALSE_OBJ))
#define object_create_null() (&NULL_OBJ)


object_int *object_create_int(long);

object_error *object_create_error(const char *, ...);

object_function *object_create_function(linked_list *, ast_block_statement *,
                                        environment *);

object_string *object_create_string(const char *, size_t);

object_builtin *object_create_builtin(builtin_fn);

object_array *object_create_array(arraylist *);

object_hash *object_create_hash(hashtable *);

object_return_value *object_create_return_value(object_object *);

object_compiled_fn *object_create_compiled_fn(instructions *, size_t, size_t);

object_closure *object_create_closure(object_compiled_fn *fn, const arraylist *);

//object_bool *get_monkey_true(void);

object_object *object_copy_object(object_object *);

void object_free(void *);

#endif // OBJECT_H
