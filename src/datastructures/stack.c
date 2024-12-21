//
// Created by dgood on 12/5/24.
//

#include "stack.h"

#include <err.h>
#include <stdlib.h>
#include <stdbool.h>


// Function to initialize the stack
stack *stack_create() {
  stack *s = malloc(sizeof(stack));
  s->top   = -1; // Empty stack
  return s;
}

// Function to check if the stack is empty
int stack_isEmpty(const stack *stack) {
  return stack->top == -1;
}

// Function to check if the stack is full
int stack_isFull(const stack *stack) {
  return stack->top == STACK_SIZE - 1;
}

// Function to push an element onto the stack
bool stack_push(stack *stack, void *value) {
  if (stack_isFull(stack)) {
    return false;
  }
  stack->data[++stack->top] = value;
  return true;
}

// Function to pop an element from the stack
void *stack_pop(stack *stack) {
  if (stack_isEmpty(stack)) {
    return NULL; // Indicate error
  }
  return stack->data[stack->top--];
}

// Function to peek at the top element without popping
void *stack_peek(const stack *stack) {
  if (stack_isEmpty(stack)) {
    return NULL;
  }
  return stack->data[stack->top];
}

void stack_free(stack *stack) {
  free(stack);
}