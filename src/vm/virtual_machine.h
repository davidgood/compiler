//
// Created by dgood on 12/7/24.
//

#ifndef VM_H
#define VM_H

#include <stdlib.h>
#include "../compiler/compiler_core.h"
#include "../datastructures/arraylist.h"
#include "../object/object.h"
#include "../opcode/opcode.h"
#include "frame.h"

#define STACKSIZE 2048
#define GLOBALS_SIZE 65536
#define MAX_FRAMES 1024

typedef enum vm_error_code {
    VM_ERROR_NONE,
    VM_STACKOVERFLOW,
    VM_UNSUPPORTED_OPERAND,
    VM_UNSUPPORTED_OPERATOR,
    VM_NON_FUNCTION,
    VM_WRONG_NUMBER_ARGUMENTS
} vm_error_code;

static const char *VM_ERROR_DESC[] = {
        "VM_ERROR_NONE",
        "STACKOVERFLOW",
        "UNSUPPORTED_OPERAND",
        "UNSUPPORTED_OPERATOR",
        "VM_NON_FUNCTION",
        "VM_WRONG_NUMBER_OF_ARGUMENTS"
};

typedef struct vm_error {
    vm_error_code code;
    char *        msg;
} vm_error;

#define get_vm_error_desc(err) VM_ERROR_DESC[err]

typedef struct virtual_machine {
    frame *        frames[MAX_FRAMES];
    size_t         frame_index;
    arraylist *    constants;
    object_object *stack[STACKSIZE];
    object_object *globals[GLOBALS_SIZE];
    size_t         sp;
} virtual_machine;

virtual_machine *vm_init(const bytecode *);

virtual_machine *vm_init_with_state(bytecode *, object_object *[GLOBALS_SIZE]);

void vm_free(virtual_machine *);

object_object *vm_last_popped_stack_elem(virtual_machine *);

vm_error vm_run(virtual_machine *);

#endif //VM_H
