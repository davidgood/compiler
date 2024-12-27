//
// Created by dgood on 12/7/24.
//

#include "frame.h"

#include <err.h>
#include <log.h>
#include <stdlib.h>

frame *frame_init(object_closure *cl, size_t bp) {
    log_debug("Entering Frame Init");
    frame *frame = malloc(sizeof(*frame));
    if (frame == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    frame->cl = (object_closure *) object_copy_object((object_object *) cl);
    if (frame->cl == NULL) {
        fprintf(stderr, "cl is null\n");
    }
    frame->ip = 0;
    frame->bp = bp;
    return frame;
}

void frame_free(frame *frame) {
    log_debug("Entering Frame Free");
    object_free(frame->cl);
    free(frame);
}
