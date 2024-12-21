//
// Created by dgood on 12/7/24.
//

#ifndef FRAME_H
#define FRAME_H
#include "../object/object.h"

typedef struct frame_t {
    object_closure *cl;
    size_t          ip;
    size_t          bp;
} frame;

frame *frame_init(object_closure *, size_t);

void frame_free(frame *);

#define get_frame_instructions(frame) frame->cl->fn->instructions
#endif //FRAME_H
