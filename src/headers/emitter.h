#ifndef UNI_EMITTER_H_INCLUDED_
#define UNI_EMITTER_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "word.h"

typedef struct uniEmitStack_t {
    size_t* items;
    size_t count;
    size_t cap;
} uniEmitStack;

typedef struct uniEmitter_t {
    FILE* out;
    size_t tmp_counter;
    size_t str_counter;
    uniEmitStack stack;
} uniEmitter;

uniEmitter* uni_createEmitter(const char* out_path);
void uni_destroyEmitter(uniEmitter* emitter);

void uni_emitOp(uniEmitter* emitter, uniOp* op);
void uni_emitBlock(uniEmitter* emitter, uniOp* block);
void uni_emitProgram(uniEmitter* emitter, uniOp* program);

static inline bool uni_emitPush(uniEmitter* emitter, size_t temp) {
    if(emitter->stack.count >= emitter->stack.cap) {
        size_t new_cap = (emitter->stack.cap == 0)? 8 : emitter->stack.cap * 2;
        size_t* new_items = realloc(emitter->stack.items, new_cap * sizeof(size_t));
        if(!new_items) return false;

        emitter->stack.items = new_items;
        emitter->stack.cap = new_cap;
    }

    emitter->stack.items[emitter->stack.count++] = temp;
    return true;
}

static inline size_t uni_emitPop(uniEmitter* emitter) {
    return emitter->stack.items[--emitter->stack.count];
}

static inline size_t uni_getFreshTemp(uniEmitter* emitter) {
    return emitter->tmp_counter++;
}

#endif
