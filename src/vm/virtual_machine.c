//
// Created by dgood on 12/7/24.
//

#include "virtual_machine.h"

#include <assert.h>
#include <conversions.h>
#include <err.h>
#include <stdarg.h>
#include <stdlib.h>
#include "../compiler/compiler_core.h"
#include "../object/builtins.h"
#include "../object/object.h"
#include "../opcode/opcode.h"
#include "frame.h"

#include <endian.h>

static char *get_err_msg(const char *s, ...) {
    char *  msg = NULL;
    va_list ap;
    va_start(ap, s);
    const int retval = vasprintf(&msg, s, ap);
    va_end(ap);
    if (retval == -1)
        err(EXIT_FAILURE, "malloc failed");
    return msg;
}

static frame *get_current_frame(const virtual_machine *vm) {
    return vm->frames[vm->frame_index - 1];
}

static instructions *get_frame_instructions(const frame *frame) {
    return frame->cl->fn->instructions;
}

//#define get_current_frame(vm) vm->frames[vm->frame_index - 1]
//#define get_frame_instructions(frame) frame->cl->fn->instructions

static void push_frame(virtual_machine *vm, struct frame_t *frame) {
    vm->frames[vm->frame_index++] = frame;
}

static frame *pop_frame(virtual_machine *vm) {
    vm->frame_index--;
    frame *f = vm->frames[vm->frame_index];
    for (size_t i = 0; i < f->cl->fn->num_locals; i++) {
        object_free(vm->stack[f->bp + i]);
    }

    return f;
}

virtual_machine *vm_init(const bytecode *bytecode) {
    virtual_machine *vm = malloc(sizeof(struct virtual_machine));
    if (vm == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    object_compiled_fn *main_fn      = object_create_compiled_fn(bytecode->instructions, 0, 0);
    object_closure *    main_closure = object_create_closure(main_fn, NULL);
    frame *             main_frame   = frame_init(main_closure, 0);
    vm->frames[0]                    = main_frame;
    vm->frame_index                  = 1;
    vm->constants                    = bytecode->constants_pool;
    vm->sp                           = 0;
    for (size_t i      = 0; i < GLOBALS_SIZE; i++)
        vm->globals[i] = NULL;

    object_free(main_closure);
    object_free(main_fn);

    return vm;
}

virtual_machine *vm_init_with_state(bytecode *bytecode, object_object *globals[GLOBALS_SIZE]) {
    virtual_machine *vm = vm_init(bytecode);
    for (size_t i = 0; i < GLOBALS_SIZE; i++) {
        if (globals[i] != NULL) {
            vm->globals[i] = object_copy_object(globals[i]);
        } else {
            break;
        }
    }
    return vm;
}

void vm_free(virtual_machine *vm) {
    for (size_t i = 0; i < vm->sp; i++) {
        object_free(vm->stack[i]);
    }
    for (size_t i = 0; i < GLOBALS_SIZE; i++) {
        if (vm->globals[i] != NULL) {
            object_free(vm->globals[i]);
        } else {
            break;
        }
    }
    for (size_t i = 0; i < vm->frame_index; i++)
        frame_free(vm->frames[i]);
    free(vm);
}

object_object *vm_last_popped_stack_elem(virtual_machine *vm) {
    return vm->stack[vm->sp];
}

#define vm_push_copy(vm, obj) vm->stack[vm->sp++] = object_copy_object(obj)
#define vm_push(vm, obj) (vm->stack[vm->sp++] = obj)


static vm_error vm_push_closure(virtual_machine *vm, size_t const_index, size_t num_free_vars) {
    vm_error       vm_err = {VM_ERROR_NONE, NULL};
    object_object *obj    = (object_object *) arraylist_get(vm->constants, const_index);
    if (obj->type != OBJECT_COMPILED_FUNCTION) {
        vm_err.code = VM_NON_FUNCTION;
        vm_err.msg  = get_err_msg("not a function: %s\n", get_type_name(obj->type));
        return vm_err;
    }
    arraylist *free_vars = arraylist_create(num_free_vars, NULL);
    for (size_t i = 0; i < num_free_vars; i++) {
        object_object *free_var = vm->stack[vm->sp - num_free_vars + i];
        arraylist_add(free_vars, free_var);
    }
    vm->sp -= num_free_vars;
    object_compiled_fn *fn      = (object_compiled_fn *) obj;
    object_closure *    closure = object_create_closure(fn, free_vars);
    arraylist_destroy(free_vars);
    vm_push(vm, (object_object *) closure);
    return vm_err;
}

static object_object *vm_pop(virtual_machine *vm) {
    object_object *obj = vm->stack[vm->sp - 1];
    vm->sp--;
    return obj;
}

static object_object *get_constant(virtual_machine *vm, size_t const_index) {
    return (object_object *) arraylist_get(vm->constants, const_index);
}

static vm_error execute_binary_int_op(virtual_machine *vm, Opcode op, long leftval, long rightval) {
    long              result;
    vm_error          error = {VM_ERROR_NONE, NULL};
    OpcodeDefinition *op_def;
    switch (op) {
        case OP_ADD:
            result = leftval + rightval;
            break;
        case OP_SUB:
            result = leftval - rightval;
            break;
        case OP_MUL:
            result = leftval * rightval;
            break;
        case OP_DIV:
            result = leftval / rightval;
            break;
        default:
            op_def = opcode_definition_lookup(op);
            error.code = VM_UNSUPPORTED_OPERATOR;
            error.msg  = get_err_msg("opcode %s not supported for integer operands", op_def->name);
            return error;
    }
    object_object *result_obj = (object_object *) object_create_int(result);
    vm_push(vm, result_obj);
    return error;
}

static vm_error execute_binary_string_op(virtual_machine *vm, Opcode op, object_string *leftval,
                                         object_string *  rightval) {
    char *   result = NULL;
    vm_error error  = {VM_ERROR_NONE, NULL};
    if (op != OP_ADD) {
        OpcodeDefinition *op_def = opcode_definition_lookup(op);
        error.code               = VM_UNSUPPORTED_OPERATOR;
        error.msg                = get_err_msg("opcode %s not support for string operands", op_def->name);
        return error;
    }
    if ((asprintf(&result, "%s%s", leftval->value, rightval->value)) == -1)
        err(EXIT_FAILURE, "malloc failed");
    object_object *result_obj = (object_object *) object_create_string(result, leftval->length + rightval->length);
    free(result);
    vm_push(vm, result_obj);
    return error;
}

static vm_error execute_binary_op(virtual_machine *vm, Opcode op) {
    object_object *right = vm_pop(vm);
    object_object *left  = vm_pop(vm);
    vm_error       vm_err;
    if (left->type == OBJECT_INT && right->type == OBJECT_INT) {
        long leftval  = ((object_int *) left)->value;
        long rightval = ((object_int *) right)->value;
        vm_err        = execute_binary_int_op(vm, op, leftval, rightval);
    } else if (left->type == OBJECT_STRING && right->type == OBJECT_STRING) {
        vm_err = execute_binary_string_op(vm, op, (object_string *) left, (object_string *) right);
    } else {
        vm_err.code              = VM_UNSUPPORTED_OPERAND;
        OpcodeDefinition *op_def = opcode_definition_lookup(op);
        vm_err.msg               = get_err_msg("'%s' operation not supported with types %s and %s",
                                               op_def->name, get_type_name(left->type), get_type_name(right->type));
    }
    object_free(left);
    object_free(right);
    return vm_err;
}

static vm_error execute_integer_comparison(virtual_machine *vm, Opcode op, long left, long right) {
    bool     result = false;
    vm_error error  = {VM_ERROR_NONE, NULL};
    switch (op) {
        case OP_GREATER_THAN:
            if (left > right)
                result = true;
            break;
        case OP_EQUAL:
            if (left == right)
                result = true;
            break;
        case OP_NOT_EQUAL:
            if (left != right)
                result = true;
            break;
        default:
            OpcodeDefinition *op_def = opcode_definition_lookup(op);
            error.code = VM_UNSUPPORTED_OPERATOR;
            error.msg  = get_err_msg("Unsupported opcode %s for integer operands", op_def->name);
            return error;
    }
    vm_push(vm, (object_object *) object_create_bool(result));
    return error;
}

static vm_error execute_bang_operator(virtual_machine *vm) {
    object_object *operand = vm_pop(vm);
    object_bool *  bool_operand;
    vm_error       vm_err;
    if (operand->type != OBJECT_BOOL && operand->type != OBJECT_NULL) {
        vm_err.code = VM_UNSUPPORTED_OPERAND;
        vm_err.msg  = get_err_msg("'!' operator not supported for %s type operands",
                                  get_type_name(operand->type));
        return vm_err;
    }
    if (operand->type == OBJECT_NULL)
        bool_operand = object_create_bool(false);
    else
        bool_operand = (object_bool *) operand;
    vm_push(vm, (object_object *) object_create_bool(!bool_operand->value));
    vm_err.code = VM_ERROR_NONE;
    vm_err.msg  = NULL;
    return vm_err;
}

static vm_error execute_minus_operator(virtual_machine *vm) {
    object_object *operand = vm_pop(vm);
    vm_error       vm_err;
    if (operand->type != OBJECT_INT) {
        vm_err.code = VM_UNSUPPORTED_OPERAND;
        vm_err.msg  = get_err_msg("'-' operator not supported for %s type operands",
                                  get_type_name(operand->type));
        return vm_err;
    }
    object_int *int_operand = (object_int *) operand;
    object_int *result      = object_create_int(-int_operand->value);
    vm_push(vm, (object_object *) result);
    object_free(operand);
    vm_err.code = VM_ERROR_NONE;
    vm_err.msg  = NULL;
    return vm_err;
}

static vm_error execute_array_index_expression(virtual_machine *vm, object_array *left, object_int *index) {
    vm_error vm_err = {VM_ERROR_NONE, NULL};
    if (index->value < 0 || index->value >= left->elements->size) {
        vm_push(vm, (object_object *) object_create_null());
        return vm_err;
    }
    vm_push_copy(vm, arraylist_get(left->elements, index->value));
    return vm_err;
}

static vm_error execute_hash_index_expression(virtual_machine *vm, object_hash *left, object_object *index) {
    const vm_error vm_err = {VM_ERROR_NONE, NULL};
    object_object *value  = hashtable_get(left->pairs, index);
    if (value == NULL)
        vm_push(vm, (object_object *) object_create_null());
    else
        vm_push_copy(vm, value);
    return vm_err;
}

static vm_error execute_index_expression(virtual_machine *vm, object_object *left, object_object *index) {
    vm_error vm_err;
    if (left->type == OBJECT_ARRAY) {
        if (index->type != OBJECT_INT) {
            vm_err.code = VM_UNSUPPORTED_OPERATOR;
            vm_err.msg  = get_err_msg("unsupported index operator type %s for array object",
                                      get_type_name(index->type));
            return vm_err;
        }
        return execute_array_index_expression(vm, (object_array *) left, (object_int *) index);
    } else if (left->type == OBJECT_HASH)
        return execute_hash_index_expression(vm, (object_hash *) left, index);
    vm_err.code = VM_UNSUPPORTED_OPERATOR;
    vm_err.msg  = get_err_msg("index operator not supported for %s", get_type_name(left->type));
    return vm_err;
}

static vm_error execute_comparison_op(virtual_machine *vm, Opcode op) {
    vm_error       error = {VM_ERROR_NONE, NULL};
    object_object *right = vm_pop(vm);
    object_object *left  = vm_pop(vm);
    if (left->type == OBJECT_INT && right->type == OBJECT_INT) {
        long leftval  = ((object_int *) left)->value;
        long rightval = ((object_int *) right)->value;
        error         = execute_integer_comparison(vm, op, leftval, rightval);
    } else if (left->type == OBJECT_BOOL && right->type == OBJECT_BOOL) {
        OpcodeDefinition *op_def;
        _Bool             result = false;
        switch (op) {
            case OP_GREATER_THAN:
                vm_push(vm, (object_object *) object_create_bool(false));
                break;
            case OP_EQUAL:
                if (left == right)
                    result = true;
                break;
            case OP_NOT_EQUAL:
                if (left != right)
                    result = true;
                break;
            default:
                op_def = opcode_definition_lookup(op);
                error.code = VM_UNSUPPORTED_OPERATOR;
                error.msg  = get_err_msg("Unsupported opcode %s", op_def->name);
                goto RETURN;
        }
        vm_push(vm, (object_object *) object_create_bool(result));
    } else {
        error.code = VM_UNSUPPORTED_OPERAND;
        error.msg  = get_err_msg("Unsupported operand types %s and %s",
                                 get_type_name(left->type), get_type_name(right->type));
    }
RETURN:
    object_free(left);
    object_free(right);
    return error;
}

static bool is_truthy(object_object *condition) {
    switch (condition->type) {
        case OBJECT_BOOL:
            return ((object_bool *) condition)->value;
        case OBJECT_NULL:
            return false;
        default:
            return true;
    }
}

static arraylist *build_array(virtual_machine *vm, size_t array_size) {
    arraylist *list = arraylist_create(array_size, NULL);
    for (size_t i = vm->sp - array_size; i < vm->sp; i++) {
        object_object *obj = (object_object *) vm->stack[i];
        arraylist_add(list, (obj));
    }
    vm->sp -= array_size;
    return list;
}

static hashtable *build_hash(const virtual_machine *vm, const size_t size) {
    assert(vm != NULL);
    assert(vm->stack != NULL);
    assert(vm->sp >= size);

    hashtable *table = hashtable_create(object_get_hash,
                                        object_equals, NULL, NULL);
    if (!table) {
        fprintf(stderr, "Error: Failed to create hashtable\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = vm->sp - size; i < vm->sp; i += 2) {
        assert(i + 1 < vm->sp); // Ensure no out-of-bounds access
        object_object *key   = vm->stack[i];
        object_object *value = vm->stack[i + 1];
        assert(key != NULL);
        assert(value != NULL);

        hashtable_set(table, key, value);
    }
    return table;
}

static vm_error call_builtin(virtual_machine *vm, object_builtin *callee, size_t num_args) {
    vm_error     vm_err;
    linked_list *args = linked_list_create();
    for (size_t i = vm->sp - num_args; i < vm->sp; i++) {
        object_object *top = vm->stack[i];
        linked_list_addNode(args, top);
    }
    object_object *result = callee->function(args);
    linked_list_free(args, NULL);
    vm_push(vm, result);
    vm_err.code = VM_ERROR_NONE;
    vm_err.msg  = NULL;
    return vm_err;
}

static vm_error call_closure(virtual_machine *vm, object_closure *closure, size_t num_args) {
    vm_error vm_err;
    if (closure->fn->num_args != num_args) {
        vm_err.code = VM_WRONG_NUMBER_ARGUMENTS;
        vm_err.msg  = get_err_msg("wrong number of arguments: want=%zu, got=%zu",
                                  closure->fn->num_args, num_args);
        return vm_err;
    }
    frame *new_frame = frame_init(closure, vm->sp - num_args);
    push_frame(vm, new_frame);
    vm->sp      = new_frame->bp + closure->fn->num_locals;
    vm_err.code = VM_ERROR_NONE;
    vm_err.msg  = NULL;
    object_free(closure);
    return vm_err;
}

static vm_error execute_call(virtual_machine *vm, size_t num_args) {
    object_object *callee = vm->stack[vm->sp - 1 - num_args];
    vm_error       vm_err;
    switch (callee->type) {
        case OBJECT_CLOSURE:
            vm_err = call_closure(vm, (object_closure *) callee, num_args);
            break;
        case OBJECT_BUILTIN:
            vm_err = call_builtin(vm, (object_builtin *) callee, num_args);
            break;
        default:
            vm_err.code = VM_NON_FUNCTION;
            vm_err.msg = get_err_msg("Calling non-function\n");
            break;
    }
    return vm_err;
}

uint16_t read_uint16(const uint8_t *bytes) {
    return (uint16_t) (bytes[0] << 8) | bytes[1];
}

vm_error vm_run(virtual_machine *vm) {
    size_t          const_index, jmp_pos, sym_index, array_size, num_elements;
    vm_error        vm_err;
    object_object * top = NULL;
    arraylist *     array_list;
    hashtable *     table;
    object_array *  array_obj;
    object_hash *   hash_obj;
    object_object * index;
    object_object * left;
    object_object * return_value;
    frame *         popped_frame = NULL;
    size_t          num_args;
    size_t          builtin_idx;
    size_t          num_free_vars;
    object_closure *current_closure;
    frame *         current_frame              = get_current_frame(vm);
    size_t          ip                         = 0;
    instructions *  current_frame_instructions = NULL;
    Opcode          op                         = OP_INVALID;
    while (current_frame->ip < get_frame_instructions(current_frame)->length) {
        ip                         = current_frame->ip;
        current_frame_instructions = get_frame_instructions(current_frame);
        op                         = current_frame_instructions->bytes[ip];
        if (top != NULL) {
            object_free(top);
            top = NULL;
        }
        switch (op) {
            case OP_CONSTANT:
                const_index = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip += 2;
                vm_push_copy(vm, get_constant(vm, const_index));
                break;
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
                vm_err = execute_binary_op(vm, op);
                if (vm_err.code != VM_ERROR_NONE)
                    return vm_err;
                break;
            case OP_POP:
                top = vm_pop(vm);
                break;
            case OP_TRUE:
                vm_push(vm, (object_object *) object_create_bool(true));
                break;
            case OP_FALSE:
                vm_push(vm, (object_object *) object_create_bool(false));
                break;
            case OP_NULL:
                vm_push(vm, (object_object *) object_create_null());
                break;
            case OP_GREATER_THAN:
            case OP_EQUAL:
            case OP_NOT_EQUAL:
                vm_err = execute_comparison_op(vm, op);
                if (vm_err.code != VM_ERROR_NONE)
                    return vm_err;
                break;
            case OP_MINUS:
                vm_err = execute_minus_operator(vm);
                if (vm_err.code != VM_ERROR_NONE)
                    return vm_err;
                break;
            case OP_BANG:
                vm_err = execute_bang_operator(vm);
                if (vm_err.code != VM_ERROR_NONE)
                    return vm_err;
                break;
            case OP_JUMP:
                jmp_pos = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip = jmp_pos - 1;
                break;
            case OP_JUMP_NOT_TRUTHY:
                jmp_pos = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip += 2;
                top = vm_pop(vm);
                if (!is_truthy(top))
                    current_frame->ip = jmp_pos - 1;
                break;
            case OP_SET_GLOBAL:
                sym_index = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip += 2;
                top                    = vm_pop(vm);
                vm->globals[sym_index] = object_copy_object(top);
                break;
            case OP_SET_LOCAL:
                sym_index = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip++;
                top                                      = vm_pop(vm);
                vm->stack[current_frame->bp + sym_index] = object_copy_object(top);
                break;
            case OP_GET_GLOBAL:
                sym_index = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip += 2;
                vm_push_copy(vm, vm->globals[sym_index]);
            // if (vm_err.code != VM_ERROR_NONE)
            //     return vm_err;
                break;
            case OP_GET_LOCAL:
                sym_index = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip++;
                vm_push_copy(vm, vm->stack[current_frame->bp + sym_index]);
                break;
            case OP_GET_FREE:
                sym_index = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip++;
                current_closure = get_current_frame(vm)->cl;
                vm_push_copy(vm, current_closure->free_variables[sym_index]);
                break;
            case OP_ARRAY:
                array_size = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip += 2;
                array_list = build_array(vm, array_size);
                array_obj  = object_create_array(array_list);
                vm_push(vm, (object_object *) array_obj);
                break;
            case OP_HASH:
                num_elements = htobe16(current_frame_instructions->bytes[ip + 1]);
            //num_elements = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip += 2;
                fprintf(stdout, "Before build hash: %p", current_frame_instructions->bytes);
                table = build_hash(vm, num_elements);
                fprintf(stdout, "After build hash: %p", current_frame_instructions->bytes);
                hash_obj = object_create_hash(table);
                vm_push(vm, (object_object *) hash_obj);
                vm->sp -= num_elements;
                break;
            case OP_INDEX:
                index = vm_pop(vm);
                left   = vm_pop(vm);
                vm_err = execute_index_expression(vm, left, index);
                object_free(index);
                object_free(left);
                if (vm_err.code != VM_ERROR_NONE)
                    return vm_err;
                break;
            case OP_CALL:
                num_args = current_frame_instructions->bytes[ip + 1];
                current_frame->ip++;
                vm_err = execute_call(vm, num_args);
                if (vm_err.code != VM_ERROR_NONE)
                    return vm_err;
                break;
            case OP_RETURN_VALUE:
                return_value = (object_object *) vm_pop(vm);
                popped_frame = pop_frame(vm);
                vm->sp       = popped_frame->bp - 1;
                vm_push(vm, return_value);
                break;
            case OP_RETURN:
                popped_frame = pop_frame(vm);
                vm->sp = popped_frame->bp - 1;
                vm_push(vm, (object_object *) object_create_null());
                break;
            case OP_GET_BUILTIN:
                builtin_idx = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                current_frame->ip++;
                const char *    builtin_name = get_builtins_name(builtin_idx);
                object_builtin *builtin      = get_builtins(builtin_name);
                vm_push(vm, (object_object *) builtin);
                break;
            case OP_CLOSURE:
                const_index = read_uint16(&current_frame_instructions->bytes[ip + 1]);
                num_free_vars = current_frame_instructions->bytes[ip + 3];
                current_frame->ip += 3;
                vm_push_closure(vm, const_index, num_free_vars);
                break;
            case OP_CURRENT_CLOSURE:
                current_closure = current_frame->cl;
                vm_push_copy(vm, (object_object *) current_closure);
                break;
            default:
                OpcodeDefinition *op_def = opcode_definition_lookup(op);
                vm_err.code = VM_UNSUPPORTED_OPERATOR;
                vm_err.msg  = get_err_msg("Unsupported opcode %s", op_def->name);
                return vm_err;
        }
        if (popped_frame == current_frame) {
            frame_free(popped_frame);
            popped_frame = NULL;
        } else
            current_frame->ip++;
        current_frame = get_current_frame(vm);
    }
    vm_err.code = VM_ERROR_NONE;
    vm_err.msg  = NULL;
    return vm_err;
}
