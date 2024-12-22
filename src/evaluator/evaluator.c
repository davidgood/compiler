//
// Created by dgood on 12/5/24.
//

#include "evaluator.h"

#include <assert.h>
#include <string.h>

#include "../ast/ast.h"
#include "../object/builtins.h"
#include "../object/environment.h"
#include "../object/object.h"

static bool is_error(object_object *obj) {
    return obj != NULL && obj->type == OBJECT_ERROR;
}

static object_object *eval_boolean_infix_expression(const char * operator,
                                                    object_bool *left_value,
                                                    object_bool *right_value) {
    bool result;
    if (strcmp(operator, "&&") == 0)
        result = left_value->value && right_value->value;
    else if (strcmp(operator, "||") == 0)
        result = left_value->value || right_value->value;
    else if (strcmp(operator, "==") == 0)
        result = left_value->value == right_value->value;
    else if (strcmp(operator, "!=") == 0)
        result = left_value->value != right_value->value;
    else
        return (object_object *) object_create_error("unknown operator: %s %s %s",
                                                     get_type_name(
                                                             left_value->object.type),
                                                     operator,
                                                     get_type_name(
                                                             right_value->object.
                                                             type));
    return (object_object *) object_create_bool(result);
}

static object_object *eval_integer_infix_expression(const char *operator,
                                                    object_int *left_value,
                                                    object_int *right_value) {
    long result;
    if (strcmp(operator, "+") == 0)
        result = left_value->value + right_value->value;
    else if (strcmp(operator, "-") == 0)
        result = left_value->value - right_value->value;
    else if (strcmp(operator, "*") == 0)
        result = left_value->value * right_value->value;
    else if (strcmp(operator, "/") == 0)
        if (right_value->value != 0)
            result = left_value->value / right_value->value;
        else
            return (object_object *)
                    object_create_error("division by 0 not allowed");
    else if (strcmp(operator, "<") == 0)
        return (object_object *)
                object_create_bool(left_value->value < right_value->value);
    else if (strcmp(operator, ">") == 0)
        return (object_object *)
                object_create_bool(left_value->value > right_value->value);
    else if (strcmp(operator, "==") == 0)
        return (object_object *)
                (object_object *) object_create_bool(
                        left_value->value == right_value->value);
    else if (strcmp(operator, "!=") == 0)
        return (object_object *)
                object_create_bool(left_value->value != right_value->value);
    else if (strcmp(operator, "%") == 0)
        if (right_value->value != 0)
            result = left_value->value % right_value->value;
        else
            return (object_object *)
                    object_create_error("division by 0 not allowed");
    else
        return (object_object *) object_create_error("unknown operator: %s %s %s",
                                                     get_type_name(
                                                             left_value->object.type),
                                                     operator,
                                                     get_type_name(
                                                             right_value->object.
                                                             type));
    return (object_object *) object_create_int(result);
}

static object_object *eval_string_infix_expression(const char *         operator,
                                                   const object_string *left_value,
                                                   const object_string *right_value) {
    if (strcmp(operator, "+") == 0) {
        const size_t new_len    = left_value->length + right_value->length;
        char *       new_string = malloc(new_len + 1);
        memcpy(new_string, left_value->value, left_value->length);
        memcpy(new_string + left_value->length, right_value->value,
               right_value->length);
        new_string[new_len]           = 0;
        object_string *new_string_obj = object_create_string(NULL, 0);
        new_string_obj->value         = new_string;
        new_string_obj->length        = new_len;
        return (object_object *) new_string_obj;
    }

    if (strcmp(operator, "==") == 0) {
        if (strcmp(left_value->value, right_value->value) == 0)
            return (object_object *) object_create_bool(true);
        else
            return (object_object *) object_create_bool(false);
    }

    if (strcmp(operator, "!=") == 0) {
        if (strcmp(left_value->value, right_value->value) == 0)
            return (object_object *) object_create_bool(false);
        else
            return (object_object *) object_create_bool(true);
    }

    return (object_object *) object_create_error("unknown operator: %s %s %s",
                                                 get_type_name(
                                                         left_value->object.type),
                                                 operator,
                                                 get_type_name(
                                                         right_value->object.type));
}

static object_object *eval_minus_prefix_expression(object_object *right_value) {
    if (right_value->type != OBJECT_INT)
        return (object_object *) object_create_error("unknown operator: -%s",
                                                     get_type_name(
                                                             right_value->type));
    const object_int *int_obj = (object_int *) right_value;
    return (object_object *) object_create_int(-(int_obj->value));
}

static object_object *eval_bang_expression(object_object *right_value) {
    if (right_value->type == OBJECT_NULL) {
        return (object_object *) object_create_null();
    }
    if (right_value->type == OBJECT_BOOL) {
        const object_bool *value = (object_bool *) right_value;
        if (value->value) {
            return (object_object *) object_create_bool(false);
        }
        return (object_object *) object_create_bool(true);
    }
    return (object_object *) object_create_bool(false);
}

static object_object *eval_prefix_epxression(const char *operator, object_object *right_value) {
    if (strcmp(operator, "!") == 0) {
        return eval_bang_expression(right_value);
    } else if (strcmp(operator, "-") == 0) {
        return eval_minus_prefix_expression(right_value);
    }
    return (object_object *) object_create_error("unknown operator: %s%s",
                                                 operator,
                                                 get_type_name(
                                                         right_value->type));
}

static object_object *eval_infix_expression(const char *   operator,
                                            object_object *left_value,
                                            object_object *right_value) {
    if (left_value->type == OBJECT_INT && right_value->type == OBJECT_INT)
        return eval_integer_infix_expression(operator,
                                             (object_int *) left_value,
                                             (object_int *) right_value);
    if (left_value->type == OBJECT_STRING && right_value->type == OBJECT_STRING)
        return eval_string_infix_expression(operator,
                                            (object_string *) left_value,
                                            (object_string *) right_value);
    if (left_value->type == OBJECT_BOOL && right_value->type == OBJECT_BOOL)
        return eval_boolean_infix_expression(operator,
                                             (object_bool *) left_value,
                                             (object_bool *) right_value);
    if (strcmp(operator, "==") == 0)
        return (object_object *)
                object_create_bool(left_value == right_value);
    if (strcmp(operator, "!=") == 0)
        return (object_object *)
                object_create_bool(left_value != right_value);
    if (left_value->type != right_value->type)
        return (object_object *) object_create_error("type mismatch: %s %s %s",
                                                     get_type_name(
                                                             left_value->type),
                                                     operator,
                                                     get_type_name(
                                                             right_value->type));
    else
        return (object_object *) object_create_error("unknown operator: %s %s %s",
                                                     get_type_name(
                                                             left_value->type),
                                                     operator,
                                                     get_type_name(
                                                             right_value->type));
}

static bool is_truthy(object_object *value) {
    switch (value->type) {
        case OBJECT_NULL:
            return false;
        case OBJECT_BOOL:
            return object_create_bool(true) == (object_bool *) value;
        default:
            return true;
    }
}

static object_object *eval_if_expression(ast_expression *exp, environment *env) {
    const ast_if_expression *if_exp          = (ast_if_expression *) exp;
    object_object *          condition_value = evaluator_eval(
            (ast_node *) if_exp->condition, env);
    if (is_error(condition_value)) {
        return condition_value;
    }
    object_object *result;
    if (is_truthy(condition_value)) {
        result = evaluator_eval((ast_node *) if_exp->consequence, env);
    } else if (if_exp->alternative != NULL) {
        result = evaluator_eval((ast_node *) if_exp->alternative, env);
    } else {
        result = (object_object *) object_create_null();
    }
    object_free(condition_value);
    return result;
}

static object_object *eval_identifier_expression(ast_expression *exp, const environment *env) {
    const ast_identifier *ident_exp = (ast_identifier *) exp;
    void *                value_obj = environment_get(env, ident_exp->value);
    if (value_obj == NULL) {
        value_obj = (void *) get_builtins(ident_exp->value);
    }
    if (value_obj == NULL) {
        return (object_object *) object_create_error(
                "identifier not found: %s", ident_exp->value);
    }
    // return a copy of the value, we don't want anyone else to a value stored in the hash table
    return object_copy_object(value_obj);
}

static arraylist *eval_expressions_to_array_list(const arraylist *expression_list,
                                                 environment *    env) {
    arraylist *values = arraylist_create(expression_list->size);
    for (size_t i = 0; i < expression_list->size; i++) {
        object_object *value = evaluator_eval((ast_node *) expression_list->body[i], env);
        if (is_error(value)) {
            arraylist_destroy(values);
            values = arraylist_create(1, object_free);
            arraylist_add(values, value);
            return values;
        }
        arraylist_add(values, value);
    }
    return values;
}

static linked_list *eval_expressions_to_linked_list(const linked_list *expression_list,
                                                    environment *      env) {
    linked_list *    values   = linked_list_create();
    const list_node *exp_node = expression_list->head;
    while (exp_node != NULL) {
        object_object *value = evaluator_eval((ast_node *) exp_node->data, env);
        if (is_error(value)) {
            linked_list_free(values, NULL);
            values = linked_list_create();
            linked_list_addNode(values, value);
            return values;
        }
        linked_list_addNode(values, value);
        exp_node = exp_node->next;
    }
    return values;
}

static object_object *apply_function(object_object *function_obj, linked_list *arguments_list) {
    object_function *function;
    object_builtin * builtin;
    environment *    extended_env;
    object_object *  function_value;
    list_node *      arg_node;
    list_node *      param_node;

    switch (function_obj->type) {
        case OBJECT_FUNCTION:
            function = (object_function *) function_obj;
            extended_env = environment_create_enclosed(function->env);
            arg_node     = arguments_list->head;
            param_node   = function->parameters->head;
            while (arg_node != NULL) {
                const ast_identifier *param = (ast_identifier *) param_node->data;
                environment_put(extended_env, strdup(param->value),
                                object_copy_object(arg_node->data));
                arg_node   = arg_node->next;
                param_node = param_node->next;
            }
            function_value = evaluator_eval((ast_node *) function->body, extended_env);
            environment_free(extended_env);
            if (function_value->type == OBJECT_RETURN_VALUE) {
                object_return_value *ret_value = (object_return_value *) function_value;
                object_object *      ret       = object_copy_object(ret_value->value);
                object_free(ret_value);
                return ret;
            }
            return function_value;
        case OBJECT_BUILTIN:
            builtin = (object_builtin *) function_obj;
            return builtin->function(arguments_list);
        default:
            return (object_object *) object_create_error(
                    "not a function: %s", get_type_name(function_obj->type));
    }
}

static object_object *eval_array_index_expression(object_object *left_value,
                                                  object_object *index_value) {
    const object_array *array_obj = (object_array *) left_value;
    const object_int *  index_obj = (object_int *) index_value;
    if (index_obj->value < 0 || index_obj->value > array_obj->elements->size -
        1) {
        return (object_object *) object_create_null();
    }

    /* we need to copy the return value because the left_value and index_value objects need to be freed */
    return object_copy_object(array_obj->elements->body[index_obj->value]);
}

static object_object *eval_string_index_expression(object_object *left_value,
                                                   object_object *index_value) {
    const object_string *string = (object_string *) left_value;
    const object_int *   index  = (object_int *) index_value;
    if (index->value < 0 || index->value > string->length - 1) {
        return (object_object *) object_create_null();
    }
    return (object_object *) object_create_string(&string->value[index->value], 1);
}

static object_object *eval_hash_index_expression(object_object *left_value,
                                                 object_object *index_value) {
    const object_hash *hash_obj = (object_hash *) left_value;
    if (index_value->hash == NULL) {
        return (object_object *) object_create_error("unusable as a hash key: %s",
                                                     get_type_name(
                                                             index_value->type));
    }
    return object_copy_object(hashtable_get(hash_obj->pairs, index_value));
}

static object_object *eval_index_expression(object_object *left_value,
                                            object_object *index_value) {
    if (left_value->type == OBJECT_ARRAY && index_value->type == OBJECT_INT) {
        return eval_array_index_expression(left_value, index_value);
    }
    if (left_value->type == OBJECT_HASH) {
        return eval_hash_index_expression(left_value, index_value);
    }
    if (left_value->type == OBJECT_STRING && index_value->type == OBJECT_INT) {
        return eval_string_index_expression(left_value, index_value);
    }
    return (object_object *) object_create_error(
            "index operator not supported: %s",
            get_type_name(left_value->type));
}

static object_object *eval_while_expression(const ast_while_expression *while_exp, environment *env) {
    object_object *result    = NULL;
    object_object *condition = evaluator_eval((ast_node *) while_exp->condition, env);
    if (is_error(condition)) {
        return condition;
    }
    while (is_truthy(condition)) {
        result = evaluator_eval((ast_node *) while_exp->body, env);
        if (is_error(result)) {
            object_free(condition);
            return result;
        }
        object_free(condition);
        condition = evaluator_eval((ast_node *) while_exp->condition, env);
        if (is_truthy(condition))
            object_free(result);
    }
    if (result == NULL) {
        return (object_object *) object_create_null();
    }
    return result;
}

static object_object *eval_hash_literal(const ast_hash_literal *hash_exp, environment *env) {
    hashtable *pairs = hashtable_create(object_get_hash,
                                        object_equals, NULL, NULL);
    arraylist *keys = hashtable_get_keys(hash_exp->pairs);
    if (keys != NULL) {
        for (size_t i = 0; i < keys->size; i++) {
            ast_expression *exp_key   = arraylist_get(keys, i);
            ast_expression *exp_value = hashtable_get(hash_exp->pairs, exp_key);
            object_object * key       = evaluator_eval((ast_node *) exp_key, env);
            if (is_error(key)) {
                hashtable_destroy(pairs);
                return key;
            }
            if (key->hash == NULL) {
                hashtable_destroy(pairs);
                return (object_object *)
                        object_create_error("unusable as a hash key: %s", get_type_name(key->type));
            }
            object_object *value = evaluator_eval((ast_node *) exp_value, env);
            if (is_error(value)) {
                object_free(key);
                hashtable_destroy(pairs);
                return value;
            }
            hashtable_set(pairs, key, value);
        }
        arraylist_destroy(keys);
    }
    return (object_object *) object_create_hash(pairs);
}

static object_object *eval_expression(ast_expression *exp, environment *env) {
    ast_integer *           int_exp;
    ast_boolean_expression *bool_exp;
    ast_prefix_expression * prefix_exp;
    ast_infix_expression *  infix_exp;
    object_object *         left_value;
    object_object *         right_value;
    object_object *         exp_value;
    object_object *         function_value;
    object_object *         call_exp_value;
    object_object *         index_exp_value;
    linked_list *           arguments_value;
    ast_function_literal *  function_exp;
    ast_call_expression *   call_exp;
    ast_string *            string_exp;
    ast_array_literal *     array_exp;
    ast_index_expression *  index_exp;
    ast_hash_literal *      hash_exp;
    ast_while_expression *  while_exp;

    switch (exp->expression_type) {
        case INTEGER_EXPRESSION:
            int_exp = (ast_integer *) exp;
            return (object_object *) object_create_int(int_exp->value);
        case BOOLEAN_EXPRESSION:
            bool_exp = (ast_boolean_expression *) exp;
            return (object_object *) object_create_bool(bool_exp->value);
        case PREFIX_EXPRESSION:
            prefix_exp = (ast_prefix_expression *) exp;
            right_value = evaluator_eval((ast_node *) prefix_exp->right, env);
            if (is_error(right_value)) {
                return right_value;
            }
            exp_value = eval_prefix_epxression(prefix_exp->operator, right_value);
            object_free(right_value);
            return exp_value;
        case INFIX_EXPRESSION:
            infix_exp = (ast_infix_expression *) exp;
            left_value = evaluator_eval((ast_node *) infix_exp->left, env);
            if (is_error(left_value)) {
                return left_value;
            }
            right_value = evaluator_eval((ast_node *) infix_exp->right, env);
            if (is_error(right_value)) {
                object_free(left_value);
                return right_value;
            }
            exp_value = eval_infix_expression(infix_exp->operator, left_value,
                                              right_value);
            object_free(left_value);
            object_free(right_value);
            return exp_value;
        case IF_EXPRESSION:
            return eval_if_expression(exp, env);
        case IDENTIFIER_EXPRESSION:
            return eval_identifier_expression(exp, env);
        case FUNCTION_LITERAL:
            function_exp = (ast_function_literal *) exp;
            return (object_object *) object_create_function(
                    function_exp->parameters,
                    function_exp->body,
                    env);
        case CALL_EXPRESSION:
            call_exp = (ast_call_expression *) exp;
            function_value = evaluator_eval((ast_node *) call_exp->function, env);
            if (is_error(function_value)) {
                return function_value;
            }
            arguments_value = eval_expressions_to_linked_list(call_exp->arguments, env);
            if (arguments_value->size == 1 &&
                is_error(arguments_value->head->data)) {
                object_free(function_value);
                exp_value = object_copy_object(arguments_value->head->data);
                linked_list_free(arguments_value, NULL);
                return exp_value;
            }
            call_exp_value = apply_function(function_value, arguments_value);
            object_free(function_value);
            linked_list_free(arguments_value, NULL);
            return call_exp_value;
        case STRING_EXPRESSION:
            string_exp = (ast_string *) exp;
            return (object_object *) object_create_string(
                    string_exp->value, string_exp->length);
        case ARRAY_LITERAL:
            array_exp = (ast_array_literal *) exp;
            arraylist *elements = eval_expressions_to_array_list(
                    array_exp->elements, env);
            if (elements->size == 1 && is_error(elements->body[0])) {
                object_free(array_exp);
                exp_value = object_copy_object(elements->body[0]);
                arraylist_destroy(elements);
                return exp_value;
            }
            return (object_object *) object_create_array(elements);
        case INDEX_EXPRESSION:
            index_exp = (ast_index_expression *) exp;
            left_value = evaluator_eval((ast_node *) index_exp->left, env);
            if (is_error(left_value)) {
                return left_value;
            }
            exp_value = evaluator_eval((ast_node *) index_exp->index, env);
            if (is_error(exp_value)) {
                object_free(left_value);
                return exp_value;
            }
            index_exp_value = eval_index_expression(left_value, exp_value);
            object_free(left_value);
            object_free(exp_value);
            return index_exp_value;
        case HASH_LITERAL:
            hash_exp = (ast_hash_literal *) exp;
            return eval_hash_literal(hash_exp, env);
        case WHILE_EXPRESSION:
            while_exp = (ast_while_expression *) exp;
            return eval_while_expression(while_exp, env);
        default:
            break;
    }
    return NULL;
}

static object_object *eval_block_statement(const ast_block_statement *block_stmt, environment *env) {
    object_object *object = NULL;
    for (size_t i = 0; i < block_stmt->statement_count; i++) {
        if (object)
            object_free(object);
        object = evaluator_eval((ast_node *) block_stmt->statements[i], env);
        if (object != NULL &&
            (object->type == OBJECT_RETURN_VALUE ||
             object->type == OBJECT_ERROR))
            return object;
    }
    return object;
}

static object_object *eval_program(const ast_program *program, environment *env) {
    object_object *object = NULL;
    for (size_t i = 0; i < program->statement_count; i++) {
        if (object) {
            object_free(object);
        }
        object = evaluator_eval((ast_node *) program->statements[i], env);
        if (object != NULL) {
            if (object->type == OBJECT_RETURN_VALUE) {
                object_return_value *return_value_object = (object_return_value *) object;
                object_object *      ret_value           = object_copy_object(return_value_object->value);
                object_free((object_object *) return_value_object);
                return ret_value;
            }
            if (object->type == OBJECT_ERROR) {
                return object;
            }
        }
    }
    return object;
}

static object_object *eval_statement(ast_statement *statement, environment *env) {
    ast_expression_statement *exp_stmt;
    ast_block_statement *     block_stmt;
    ast_return_statement *    ret_stmt;
    ast_let_statement *       let_stmt;
    object_object *           evaluated;
    switch (statement->statement_type) {
        case EXPRESSION_STATEMENT:
            exp_stmt = (ast_expression_statement *) statement;
            return eval_expression(exp_stmt->expression, env);
        case BLOCK_STATEMENT:
            block_stmt = (ast_block_statement *) statement;
            return eval_block_statement(block_stmt, env);
        case RETURN_STATEMENT:
            ret_stmt = (ast_return_statement *) statement;
            evaluated = evaluator_eval((ast_node *) ret_stmt->return_value, env);
            if (is_error(evaluated))
                return evaluated;
            return (object_object *) object_create_return_value(evaluated);
        case LET_STATEMENT:
            let_stmt = (ast_let_statement *) statement;
            evaluated = evaluator_eval((ast_node *) let_stmt->value, env);
            if (is_error(evaluated))
                return evaluated;
            if (evaluated == NULL)
                evaluated = (object_object *) object_create_null();
            environment_put(env, strdup(let_stmt->name->value), evaluated);
        default:
            break;
    }
    return NULL;
}

object_object *evaluator_eval(ast_node *node, environment *env) {
    ast_program *program;
    switch (node->type) {
        case STATEMENT:
            return eval_statement((ast_statement *) node, env);
            break;
        case EXPRESSION:
            return eval_expression((ast_expression *) node, env);
        case PROGRAM:
            program = (ast_program *) node;
            return eval_program(program, env);
    }
}
