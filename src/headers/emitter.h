#ifndef UNI_EMITTER_H_INCLUDED_
#define UNI_EMITTER_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

typedef struct uniEmitter_t {
    FILE* out;
    size_t tmp_counter;
} uniEmitter;

uniEmitter* uni_createEmitter(const char* out_path);
void uni_destroyEmitter(uniEmitter* emitter);

void uni_emitOp(uniEmitter* emitter, uniOp* op, size_t stack, size_t sp);
void uni_emitBlock(uniEmitter* emitter, uniOp* block, size_t stack, size_t sp);
void uni_emitProgram(uniEmitter* emitter, uniOp* program);

#endif
