//
// Created by dgood on 12/5/24.
//

#ifndef STACK_H
#define STACK_H
#include <stdbool.h>

#define STACK_SIZE 100 // Define the maximum size of the stack

// Stack structure
typedef struct {
  void *data[STACK_SIZE]; // Array to store stack elements
  int   top;              // Index of the top element
} stack;

stack *stack_create();

int stack_isEmpty(const stack *stack);

int stack_isFull(const stack *stack);

bool stack_push(stack *, void *);

void *stack_pop(stack *);

void *stack_peek(const stack *stack);

void stack_free(stack *stack);

#endif //STACK_H
