#ifndef UNI_EMITTER_H_INCLUDED_
#define UNI_EMITTER_H_INCLUDED_

#include <stdio.h>
#include <string.h>

#include <llvm-c/Core.h>

#include "parser.h"
#include "word.h"

typedef struct uniEmitStack_t {
    LLVMValueRef* items;
    size_t count;
    size_t cap;
} uniEmitStack;

typedef struct uniEmitBinding_t {
    const char* name;
    size_t name_len;
    LLVMValueRef ptr;
} uniEmitBinding;

typedef struct uniEmitter_t {
    LLVMContextRef ctx;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMValueRef func;                      // Current function

    LLVMValueRef* str_globals;
    size_t str_count;
    size_t str_cap;

    uniEmitStack stack;

    uniEmitBinding* bindings;
    size_t num_bindings;
    size_t bindings_cap;

    LLVMTypeRef printf_type;
    LLVMValueRef printf_fn;                 // Handle to extern printf
    LLVMValueRef fmt_int, fmt_str, fmt_flt;
} uniEmitter;

uniEmitter* uni_createEmitter(void);
void uni_destroyEmitter(uniEmitter* emitter);

bool uni_writeProgram(uniEmitter* emitter, const char* out_path);

void uni_emitOp(uniEmitter* emitter, uniOp* op);
void uni_emitBlock(uniEmitter* emitter, uniOp* block);
void uni_emitProgram(uniEmitter* emitter, uniOp* program);

static inline bool uni_emitPush(uniEmitter* emitter, LLVMValueRef val) {
    if(emitter->stack.count >= emitter->stack.cap) {
        size_t new_cap = (emitter->stack.cap == 0)? 8 : emitter->stack.cap * 2;
        LLVMValueRef* new_items = realloc(emitter->stack.items, new_cap * sizeof(LLVMValueRef));
        if(!new_items) return false;

        emitter->stack.items = new_items;
        emitter->stack.cap = new_cap;
    }

    emitter->stack.items[emitter->stack.count++] = val;
    return true;
}

static inline LLVMValueRef uni_emitPop(uniEmitter* emitter) {
    return emitter->stack.items[--emitter->stack.count];
}

#endif
