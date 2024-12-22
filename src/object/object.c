//
// Created by dgood on 12/4/24.
//

#include "object.h"
#include <err.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../ast/ast.h"
#include "../datastructures/arraylist.h"
#include "../datastructures/conversions.h"
#include "../datastructures/linked_list.h"
#include "../parser/parser.h"
#include "environment.h"
#include "opcode.h"

const object_bool TRUE_OBJ = {
        {OBJECT_BOOL, inspect, object_get_hash, object_equals, 1}, true};
const object_bool FALSE_OBJ = {
        {OBJECT_BOOL, inspect, object_get_hash, object_equals, 1}, false};
const object_null NULL_OBJ = {
        {OBJECT_NULL, inspect, NULL, NULL, 1}};

static char *function_inspect(object_object *obj) {
    object_function * function   = (object_function *) obj;
    char *     str        = NULL;
    char *     params_str = join_parameters_list(function->parameters);
    char *     body_str   = function->body->statement.node.string(function->body);
    const int  ret        = asprintf(&str, "fn(%s) {\n%s\n}", params_str, body_str);
    free(params_str);
    free(body_str);
    if (ret == -1) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return str;
}

static char *join_expressions_list(arraylist *list) {
    char *string = NULL;
    char *temp   = NULL;
    int   ret;
    for (size_t i = 0; i < list->size; i++) {
        object_object * elem        = (object_object *) list->body[i];
        char *     elem_string = elem->inspect(elem);
        if (string == NULL) {
            ret = asprintf(&temp, "%s", elem_string);
        } else {
            ret = asprintf(&temp, "%s, %s", string, elem_string);
            free(string);
        }
        free(elem_string);
        if (ret == -1) {
            err(EXIT_FAILURE, "malloc failed");
        }
        string = temp;
        temp   = NULL;
    }
    return string;
}

static char *join_expressions_table(hashtable *table) {
    char *string = NULL;
    char *temp   = NULL;
    int   ret;
    for (size_t i = 0; i < table->table_size; i++) {
        size_t *            index      = (size_t *) table->used_slots->body[i];
        linked_list *entry_list = table->table[*index];
        list_node *           entry_node = entry_list->head;
        while (entry_node != NULL) {
            hashtable_entry *entry      = (hashtable_entry *) entry_node->data;
            entry_node                  = entry_node->next;
            object_object *key_obj      = (object_object *) entry->key;
            object_object *value_obj    = (object_object *) entry->value;
            char *         key_string   = key_obj->inspect(key_obj);
            char *         value_string = value_obj->inspect(value_obj);
            if (string == NULL) {
                ret = asprintf(&temp, "%s: %s", key_string, value_string);
            } else {
                ret = asprintf(&temp, "%s, %s: %s", string, key_string, value_string);
                free(string);
            }
            free(key_string);
            free(value_string);
            if (ret == -1) {
                err(EXIT_FAILURE, "malloc failed");
            }
            string = temp;
            temp   = NULL;
        }
    }
    ret = asprintf(&temp, "{%s}", string);
    free(string);
    if (ret == -1) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return temp;
}

char *inspect(object_object *obj) {
    object_int *         int_obj;
    object_bool *        bool_obj;
    object_return_value *ret_obj;
    object_error *       err_obj;
    object_array *       array;
    object_hash *        hash_obj;
    object_compiled_fn * compiled_fn;
    object_closure *     closure;
    char *               string          = NULL;
    char *               elements_string = NULL;
    int                  ret;

    switch (obj->type) {
        case OBJECT_INT:
            int_obj = (object_int *) obj;
            return long_to_string(int_obj->value);
        case OBJECT_BOOL:
            bool_obj = (object_bool *) obj;
            return bool_obj->value ? strdup("true") : strdup("false");
        case OBJECT_NULL:
            return strdup("null");
        case OBJECT_RETURN_VALUE:
            ret_obj = (object_return_value *) obj;
            return ret_obj->value->inspect(ret_obj->value);
        case OBJECT_ERROR:
            err_obj = (object_error *) obj;
            return strdup(err_obj->message);
        case OBJECT_FUNCTION:
            return function_inspect(obj);
        case OBJECT_STRING:
            return strdup(((object_string *) obj)->value);
        case OBJECT_BUILTIN:
            return strdup("builtin function");
        case OBJECT_ARRAY:
            array = (object_array *) obj;
            if (array->elements->size > 0) {
                elements_string = join_expressions_list(array->elements);
            }
            ret = asprintf(&string, "[%s]", elements_string ? elements_string : "");
            if (elements_string != NULL) {
                free(elements_string);
            }
            if (ret == -1) {
                err(EXIT_FAILURE, "malloc failed");
            }
            return string;
        case OBJECT_HASH:
            hash_obj = (object_hash *) obj;
            return join_expressions_table(hash_obj->pairs);
        case OBJECT_COMPILED_FUNCTION:
            compiled_fn = (object_compiled_fn *) obj;
            ret = asprintf(&string, "compiled function %p", compiled_fn);
            if (ret == -1) {
                err(EXIT_FAILURE, "malloc failed");
            }
            return string;
        case OBJECT_CLOSURE:
            closure = (object_closure *) obj;
            ret = asprintf(&string, "closure[%p]", closure);
            if (ret == -1) {
                err(EXIT_FAILURE, "malloc failed");
            }
            return string;
    }
}

static bool array_equals(object_array *arr1, object_array *arr2) {
    if (arr1->elements->size != arr2->elements->size) {
        return false;
    }
    for (size_t i = 0; i < arr1->elements->size; i++) {
        if (!object_equals(arr1->elements->body[i], arr2->elements->body[i])) {
            return false;
        }
    }
    return true;
}

static bool hash_equals(object_hash *hash1, object_hash *hash2) {
    if (hash1->pairs->key_count != hash2->pairs->key_count) {
        return false;
    }
    for (size_t i = 0; i < hash1->pairs->key_count; i++) {
        size_t * index1 = (size_t *) hash1->pairs->used_slots->body[i];
        size_t * index2 = (size_t *) hash2->pairs->used_slots->body[i];
        if (*index1 != *index2) {
            /** since we are using same hash functions, and both tables
             *  are supposed to have equal length, they must have same
             *  objects at same indices
             */
            return false;
        }
    }
    return true;
}

static bool instructions_equals(const instructions *ins1,
                                const instructions *ins2) {
    if (ins1->length != ins2->length) {
        return false;
    }
    for (size_t i = 0; i < ins1->length; i++) {
        if (ins1->bytes[i] != ins2->bytes[i])
            return false;
    }
    return true;
}

bool object_equals(void *o1, void *o2) {
    object_object * obj1 = o1;
    object_object * obj2 = o2;
    if (obj1->type != obj2->type) {
        return false;
    }

    object_array *       array1;
    object_array *       array2;
    object_builtin *     builtin1;
    object_builtin *     builtin2;
    object_error *       err1;
    object_error *       err2;
    object_function *    function1;
    object_function *    function2;
    object_hash *        hash1;
    object_hash *        hash2;
    object_int *         int1;
    object_int *         int2;
    object_string *      str1;
    object_string *      str2;
    object_return_value *ret1;
    object_return_value *ret2;
    object_compiled_fn * fn1;
    object_compiled_fn * fn2;
    object_closure *     closure1;
    object_closure *     closure2;
    switch (obj1->type) {
        case OBJECT_ARRAY:
            array1 = (object_array *) obj1;
            array2 = (object_array *) obj2;
            return array_equals(array1, array2);
        case OBJECT_BOOL:
            return ((object_bool *) obj1)->value == ((object_bool *) obj2)->value;
        case OBJECT_BUILTIN:
            builtin1 = (object_builtin *) obj1;
            builtin2 = (object_builtin *) obj2;
            return builtin1 == builtin2; // these are static pointers
        case OBJECT_ERROR:
            err1 = (object_error *) obj1;
            err2 = (object_error *) obj2;
            return strcmp(err1->message, err2->message) == 0;
        case OBJECT_FUNCTION:
            function1 = (object_function *) obj1;
            function2 = (object_function *) obj2;
            return function1 == function2; //should we bother about this?
        case OBJECT_HASH:
            hash1 = (object_hash *) obj1;
            hash2 = (object_hash *) obj2;
            return hash_equals(hash1, hash2);
        case OBJECT_INT:
            int1 = (object_int *) obj1;
            int2 = (object_int *) obj2;
            return int1->value == int2->value;
        case OBJECT_STRING:
            str1 = (object_string *) obj1;
            str2 = (object_string *) obj2;
            return strcmp(str1->value, str2->value) == 0;
        case OBJECT_RETURN_VALUE:
            ret1 = (object_return_value *) obj1;
            ret2 = (object_return_value *) obj2;
            return object_equals(ret1->value, ret2->value);
        case OBJECT_NULL:
            return obj1 == obj2;
        case OBJECT_COMPILED_FUNCTION:
            fn1 = (object_compiled_fn *) obj1;
            fn2 = (object_compiled_fn *) obj2;
            return instructions_equals(fn1->instructions, fn2->instructions);
        case OBJECT_CLOSURE:
            closure1 = (object_closure *) obj1;
            closure2 = (object_closure *) obj2;
            if (!object_equals(closure1->fn, closure2->fn)) {
                return false;
            }
            if (closure1->free_variables_count != closure2->free_variables_count) {
                return false;
            }
            for (size_t i = 0; i < closure1->free_variables_count; i++) {
                if (!object_equals(closure1->free_variables[i],
                                   closure2->free_variables[i]))
                    return false;
            }
            return true;
    }
}


size_t object_get_hash(void *object) {
    object_object *    obj = object;
    object_string *str_obj;
    object_int *   int_obj;
    object_bool *  bool_obj;
    switch (obj->type) {
        case OBJECT_STRING:
            str_obj = (object_string *) obj;
            return string_hash_function(str_obj->value);
        case OBJECT_INT:
            int_obj = (object_int *) obj;
            return int_hash_function(&int_obj->value);
        case OBJECT_BOOL:
            bool_obj = (object_bool *) obj;
            return pointer_hash_function(bool_obj);
        default:
            return 0;
    }
}

object_closure *object_create_closure(object_compiled_fn *fn,
                                      const arraylist *   free_variables) {
    object_closure *closure = malloc(sizeof(*closure));
    if (closure == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    fn->object.refcount++;
    closure->fn = fn;
    if (free_variables != NULL) {
        for (size_t i                  = 0; i < free_variables->size; i++)
            closure->free_variables[i] = (object_object *) free_variables->body[i];
        closure->free_variables_count = free_variables->size;
    } else {
        closure->free_variables_count = 0;
    }
    closure->object.inspect  = inspect;
    closure->object.type     = OBJECT_CLOSURE;
    closure->object.hash     = NULL;
    closure->object.equals   = object_equals;
    closure->object.refcount = 1;
    return closure;
}

object_int *object_create_int(const long value) {
    object_int *int_obj = malloc(sizeof(object_int));
    if (int_obj == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    int_obj->object.inspect  = inspect;
    int_obj->object.type     = OBJECT_INT;
    int_obj->object.hash     = object_get_hash;
    int_obj->object.equals   = object_equals;
    int_obj->value           = value;
    int_obj->object.refcount = 1;
    return int_obj;
}

object_compiled_fn *object_create_compiled_fn(instructions *ins,
                                              const size_t  num_locals,
                                              const size_t  num_args) {
    object_compiled_fn *compiled_fn = malloc(sizeof(*compiled_fn));
    if (compiled_fn == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    compiled_fn->instructions    = ins;
    compiled_fn->num_locals      = num_locals;
    compiled_fn->num_args        = num_args;
    compiled_fn->object.type     = OBJECT_COMPILED_FUNCTION;
    compiled_fn->object.inspect  = inspect;
    compiled_fn->object.equals   = object_equals;
    compiled_fn->object.hash     = NULL;
    compiled_fn->object.refcount = 1;
    return compiled_fn;
}


object_return_value *object_create_return_value(object_object *value) {
    object_return_value *ret = malloc(sizeof(*ret));
    if (ret == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    ret->value        = value;
    ret->obj.type     = OBJECT_RETURN_VALUE;
    ret->obj.inspect  = inspect;
    ret->obj.equals   = object_equals;
    ret->obj.hash     = NULL;
    ret->obj.refcount = 1;
    return ret;
}

object_error *object_create_error(const char *fmt, ...) {
    char *        message = NULL;
    object_error *error   = malloc(sizeof(*error));
    if (error == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    error->object.type    = OBJECT_ERROR;
    error->object.inspect = inspect;
    error->object.hash    = NULL;
    error->object.equals  = object_equals;
    va_list args;
    va_start(args, fmt);
    const int ret = vasprintf(&message, fmt, args);
    if (ret == -1) {
        err(EXIT_FAILURE, "malloc failed");
    }
    va_end(args);

    error->message         = message;
    error->object.refcount = 1;
    return error;
}

static void free_int_object(object_int *int_obj) {
    if (int_obj == NULL) {
        return;
    }
    int_obj->object.equals = NULL;
    int_obj->object.hash   = NULL;
    int_obj->object.inspect = NULL;
    int_obj->object.refcount == 0;
    free(int_obj);
    int_obj = NULL;
}

static void free_function_object(object_function *function_obj) {
    free_statement((ast_statement *) function_obj->body);
    linked_list_free(function_obj->parameters, NULL);
    free(function_obj);
}

static void free_compiled_function_object(object_compiled_fn *compiled_fn) {
    if (compiled_fn == NULL) {
        return;
    }
    if (compiled_fn->instructions != NULL) {
        instructions_free(compiled_fn->instructions);
    }
    free(compiled_fn);
}

static void free_error_object(object_error *err_obj) {
    free(err_obj->message);
    free(err_obj);
}

static void free_return_object(object_return_value *ret_obj) {
    free(ret_obj->value);
    free(ret_obj);
}

static void free_builtin_object(object_builtin *builtin_obj) {
    if (builtin_obj == NULL) {
        return;
    }
    builtin_obj->object.equals = NULL;
    builtin_obj->object.hash   = NULL;
    builtin_obj->object.inspect = NULL;
    builtin_obj->object.refcount == 0;
    free(builtin_obj);
    builtin_obj = NULL;
}

static void free_string_object(object_string *str_obj) {
    free(str_obj->value);
    free(str_obj);
}

static void free_array_object(object_array *array_obj) {
    for (size_t i = 0; i < array_obj->elements->size; i++) {
        object_free(array_obj->elements->body[i]);
    }
    arraylist_destroy(array_obj->elements);
    free(array_obj);
}

static void free_hash_object(object_hash *hash_obj) {
    arraylist *keys = hashtable_get_keys(hash_obj->pairs);
    if (keys != NULL) {
        for (size_t i = 0; i < keys->size; i++) {
            object_object * k = keys->body[i];
            object_object * val = hashtable_get(hash_obj->pairs, k);
            object_free(k);
            object_free(val);
        }
    }
    hashtable_destroy(hash_obj->pairs);
    free(hash_obj);
    arraylist_destroy(keys);
}

static void free_closure_object(object_closure *closure) {
    closure->fn->object.refcount--;
    if (closure->fn->object.refcount == 0) {
        free_compiled_function_object(closure->fn);
    }
    for (size_t i = 0; i < closure->free_variables_count; i++) {
        object_free(closure->free_variables[i]);
    }
    free(closure);
}

void object_free(void *v) {
    if (!v) {
        return;
    }
    object_object *object = v;
    switch (object->type) {
        case OBJECT_BUILTIN:
            object->refcount--;
            if (object->refcount == 0) {
                free_builtin_object((object_builtin *) object);
            }
            break;
        case OBJECT_INT:
            object->refcount--;
            if (object->refcount == 0) {
                free_int_object((object_int *) object);
            }
            break;
        case OBJECT_ERROR:
            object->refcount--;
            if (object->refcount == 0) {
                free_error_object((object_error *)object);
            }
            break;
        case OBJECT_FUNCTION:
            object->refcount--;
            if (object->refcount == 0) {
                free_function_object((object_function *) object);
            }
        break;
        case OBJECT_RETURN_VALUE:
            object->refcount--;
            if (object->refcount == 0) {
                free_return_object((object_return_value *) object);
            }
            break;
        case OBJECT_STRING:
            object->refcount--;
            if (object->refcount == 0) {
                free_string_object((object_string *) object);
            }
            break;
        case OBJECT_ARRAY:
            object->refcount--;
            if (object->refcount == 0) {
                free_array_object((object_array *) object);
            }
            break;
        case OBJECT_HASH:
            object->refcount--;
            if (object->refcount == 0) {
                free_hash_object((object_hash *) object);
            }
            break;
        case OBJECT_COMPILED_FUNCTION:
            object->refcount--;
            if (object->refcount == 0) {
                free_compiled_function_object((object_compiled_fn *) object);
            }
            break;
        case OBJECT_CLOSURE:
            object->refcount--;
            if (object->refcount == 0) {
                free_closure_object((object_closure *) object);
            }
            break;
        default:
            break;
    }
}

static void *_copy_object(void *v) {
    return object_copy_object(v);
}

object_object *object_copy_object(object_object *object) {
    if (!object) {
        return (object_object *) object_create_null();
    }

    if (object->type == OBJECT_BOOL ||
        object->type == OBJECT_NULL ||
        object->type == OBJECT_BUILTIN) {
        return object;
    }

    object->refcount++;
    if (object->type == OBJECT_ARRAY) {
        object_array *array_obj = (object_array *) object;
        for (size_t i = 0; i < array_obj->elements->size; i++) {
            object_object *o = arraylist_get(array_obj->elements, i);
            object_copy_object(o);
        }
    }

    if (object->type == OBJECT_HASH) {
        object_hash *hash = (object_hash *) object;
        arraylist *  keys = hashtable_get_keys(hash->pairs);
        for (size_t i = 0; i < keys->size; i++) {
            object_object *k = keys->body[i];
            object_object *v = hashtable_get(hash->pairs, (void *) k);
            object_copy_object(v);
            object_copy_object(k);
        }
        arraylist_destroy(keys);
    }

    if (object->type == OBJECT_CLOSURE) {
        object_closure *closure = (object_closure *) object;
        closure->fn->object.refcount++;
        for (size_t i = 0; i < closure->free_variables_count; i++) {
            object_copy_object(closure->free_variables[i]);
        }
    }

    return object;
}


object_function *object_create_function(linked_list *parameters,
                                        ast_block_statement * body,
                                        environment *         env) {
    object_function *function = malloc(sizeof(*function));
    if (function == NULL)
        err(EXIT_FAILURE, "malloc failed");
    function->parameters      = copy_parameters(parameters);
    function->body            = (ast_block_statement *) copy_statement((ast_statement *) body);
    function->env             = env;
    function->object.type     = OBJECT_FUNCTION;
    function->object.inspect  = inspect;
    function->object.hash     = NULL;
    function->object.equals   = object_equals;
    function->object.refcount = 1;
    return function;
}

object_string *object_create_string(const char *value, const size_t length) {
    object_string *string_obj = malloc(sizeof(*string_obj));
    if (string_obj == NULL)
        err(EXIT_FAILURE, "malloc failed");
    if (value != NULL) {
        string_obj->value = malloc(sizeof(*value) * (length + 1));
        if (string_obj->value == NULL) {
            err(EXIT_FAILURE, "malloc failed");
        }
        memcpy(string_obj->value, value, length);
        string_obj->value[length] = 0;
        if (string_obj->value == NULL)
            err(EXIT_FAILURE, "malloc failed");
        string_obj->length = length;
    } else {
        string_obj->value  = NULL;
        string_obj->length = 0;
    }
    string_obj->object.type     = OBJECT_STRING;
    string_obj->object.hash     = object_get_hash;
    string_obj->object.inspect  = inspect;
    string_obj->object.equals   = object_equals;
    string_obj->object.refcount = 1;
    return string_obj;
}

object_builtin *object_create_builtin(builtin_fn function) {
    object_builtin *builtin = malloc(sizeof(*builtin));
    if (builtin == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    builtin->object.type     = OBJECT_BUILTIN;
    builtin->object.inspect  = inspect;
    builtin->object.hash     = NULL;
    builtin->function        = function;
    builtin->object.refcount = 1;
    return builtin;
}

object_array *object_create_array(arraylist *elements) {
    object_array *array = malloc(sizeof(*array));
    if (array == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    array->object.type     = OBJECT_ARRAY;
    array->object.inspect  = inspect;
    array->object.hash     = NULL;
    array->elements        = elements;
    array->object.equals   = object_equals;
    array->object.refcount = 1;
    return array;
}

object_hash *object_create_hash(hashtable *pairs) {
    object_hash *hash_obj = malloc(sizeof(*hash_obj));
    if (hash_obj == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    hash_obj->object.type     = OBJECT_HASH;
    hash_obj->object.inspect  = inspect;
    hash_obj->object.hash     = NULL;
    hash_obj->object.equals   = object_equals;
    hash_obj->pairs           = pairs;
    hash_obj->object.refcount = 1;
    return hash_obj;
}
