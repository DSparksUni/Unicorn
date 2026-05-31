#include "headers/word.h"
#include "headers/emitter.h"

void emit_add(uniEmitter* emitter);
void emit_sub(uniEmitter* emitter);
void emit_mult(uniEmitter* emitter);
void emit_div(uniEmitter* emitter);
void emit_printi(uniEmitter* emitter);
void emit_prints(uniEmitter* emitter);
void emit_eq(uniEmitter* emitter);
void emit_neq(uniEmitter* emitter);
void emit_lt(uniEmitter* emitter);
void emit_gt(uniEmitter* emitter);
void emit_le(uniEmitter* emitter);
void emit_ge(uniEmitter* emitter);
void emit_dup(uniEmitter* emitter);
void emit_drop(uniEmitter* emitter);
void emit_swap(uniEmitter* emitter);
void emit_over(uniEmitter* emitter);

static uniType INT1[] = { UNI_TYPE_INT };
static uniType INT2[] = { UNI_TYPE_INT, UNI_TYPE_INT };
static uniType STR1[] = { UNI_TYPE_STRING };

static uniType VAR_A[] = { UNI_TYPE_VAR(0) };
static uniType VAR_AA[] = { UNI_TYPE_VAR(0), UNI_TYPE_VAR(0) };
static uniType VAR_AB[] = { UNI_TYPE_VAR(0), UNI_TYPE_VAR(1) };
static uniType VAR_BA[] = { UNI_TYPE_VAR(1), UNI_TYPE_VAR(0) };
static uniType VAR_ABA[] = { UNI_TYPE_VAR(0), UNI_TYPE_VAR(1), UNI_TYPE_VAR(0) };

#define NUM_WORDS 16
static uniWord WORDS[NUM_WORDS] = {
    {"+", INT2, 2, INT1, 1, emit_add, NULL},
    {"-", INT2, 2, INT1, 1, emit_sub, NULL},
    {"*", INT2, 2, INT1, 1, emit_mult, NULL},
    {"/", INT2, 2, INT1, 1, emit_div, NULL},
    {"printi", INT1, 1, NULL, 0, emit_printi, NULL},
    {"prints", STR1, 1, NULL, 0, emit_prints, NULL},
    {"==", INT2, 2, INT1, 1, emit_eq, NULL},
    {"!=", INT2, 2, INT1, 1, emit_neq, NULL},
    {"<", INT2, 2, INT1, 1, emit_lt, NULL},
    {">", INT2, 2, INT1, 1, emit_gt, NULL},
    {"<=", INT2, 2, INT1, 1, emit_le, NULL},
    {">=", INT2, 2, INT1, 1, emit_ge, NULL},
    {"dup", VAR_A, 1, VAR_AA, 2, emit_dup, NULL},
    {"drop", VAR_A, 1, NULL, 0, emit_drop, NULL},
    {"swap", VAR_AB, 2, VAR_BA, 2, emit_swap, NULL},
    {"over", VAR_AB, 2, VAR_ABA, 3, emit_over, NULL},
};

static size_t NUM_CUSTOM_WORDS = 0;
static size_t CUSTOM_WORD_CAP = 0;
static uniWord* CUSTOM_WORDS = NULL;

void uni_registerWord(uniWord word) {
    if(NUM_CUSTOM_WORDS >= CUSTOM_WORD_CAP) {
        size_t new_cap = (CUSTOM_WORD_CAP == 0)? 8 : CUSTOM_WORD_CAP * 2;
        uniWord* new_words = realloc(CUSTOM_WORDS, new_cap * sizeof(uniWord));
        if(!new_words) return;

        CUSTOM_WORD_CAP = new_cap;
        CUSTOM_WORDS = new_words;
    }

    CUSTOM_WORDS[NUM_CUSTOM_WORDS++] = word;
}

uniWord* uni_lookupWord(const char* name, size_t len) {
    for(size_t i = 0; i < NUM_WORDS; i++) {
        if(
            strlen(WORDS[i].name) == len &&
            strncmp(WORDS[i].name, name, len) == 0
        ) return &WORDS[i];
    }

    for(size_t i = 0; i < NUM_CUSTOM_WORDS; i++) {
        if(
            strlen(CUSTOM_WORDS[i].name) == len &&
            strncmp(CUSTOM_WORDS[i].name, name, len) == 0
        ) return &CUSTOM_WORDS[i];
    }

    return NULL;
}

void uni_cleanupWords(void) {
    for(size_t i = 0; i < NUM_CUSTOM_WORDS; i++) {
        uniWord* word = &CUSTOM_WORDS[i];

        free(word->name);
        free(word->inputs);
        free(word->outputs);
    }

    free(CUSTOM_WORDS);
}

void emit_add(uniEmitter* emitter) {
    LLVMValueRef b = uni_emitPop(emitter);
    LLVMValueRef a = uni_emitPop(emitter);

    LLVMValueRef r = LLVMBuildAdd(emitter->builder, a, b, "add");

    uni_emitPush(emitter, r);
}

void emit_sub(uniEmitter* emitter) {
    LLVMValueRef b = uni_emitPop(emitter);
    LLVMValueRef a = uni_emitPop(emitter);

    LLVMValueRef r = LLVMBuildSub(emitter->builder, a, b, "sub");

    uni_emitPush(emitter, r);
}

void emit_mult(uniEmitter* emitter) {
    LLVMValueRef b = uni_emitPop(emitter);
    LLVMValueRef a = uni_emitPop(emitter);

    LLVMValueRef r = LLVMBuildMul(emitter->builder, a, b, "mul");

    uni_emitPush(emitter, r);
}

void emit_div(uniEmitter* emitter) {
    LLVMValueRef b = uni_emitPop(emitter);
    LLVMValueRef a = uni_emitPop(emitter);

    LLVMValueRef r = LLVMBuildSDiv(emitter->builder, a, b, "div");

    uni_emitPush(emitter, r);
}

void emit_printi(uniEmitter* emitter) {
    LLVMValueRef a = uni_emitPop(emitter);

    LLVMValueRef zero = LLVMConstInt(
        LLVMInt64TypeInContext(emitter->ctx),
        0, false
    );
    LLVMValueRef indicies[] = {zero, zero};
    LLVMValueRef ptr = LLVMBuildGEP2(
        emitter->builder,
        LLVMGlobalGetValueType(emitter->fmt_int),
        emitter->fmt_int,
        indicies, 2,
        "strptr"
    );

    LLVMValueRef printf_args[] = {ptr, a};
    LLVMBuildCall2(
        emitter->builder,
        emitter->printf_type,
        emitter->printf_fn,
        printf_args, 2,
        "printi"
    );
}

void emit_prints(uniEmitter* emitter) {
    LLVMValueRef a = uni_emitPop(emitter);

    LLVMValueRef zero = LLVMConstInt(
        LLVMInt64TypeInContext(emitter->ctx),
        0, false
    );
    LLVMValueRef indicies[] = {zero, zero};
    LLVMValueRef ptr = LLVMBuildGEP2(
        emitter->builder,
        LLVMGlobalGetValueType(emitter->fmt_str),
        emitter->fmt_str,
        indicies, 2,
        "strptr"
    );

    LLVMValueRef printf_args[] = {ptr, a};
    LLVMBuildCall2(
        emitter->builder,
        emitter->printf_type,
        emitter->printf_fn,
        printf_args, 2,
        "prints"
    );
}

static void emit_icmp(uniEmitter* emitter, LLVMIntPredicate pred) {
    LLVMValueRef b = uni_emitPop(emitter);
    LLVMValueRef a = uni_emitPop(emitter);

    LLVMValueRef boolean = LLVMBuildICmp(emitter->builder, pred, a, b, "cmp");
    LLVMValueRef r = LLVMBuildZExt(
        emitter->builder, boolean, LLVMInt64TypeInContext(emitter->ctx), "ext"
    );

    uni_emitPush(emitter, r);
}

void emit_eq(uniEmitter* emitter) {
    emit_icmp(emitter, LLVMIntEQ);
}

void emit_neq(uniEmitter* emitter) {
    emit_icmp(emitter, LLVMIntNE);
}

void emit_lt(uniEmitter* emitter) {
    emit_icmp(emitter, LLVMIntSLT);
}

void emit_gt(uniEmitter* emitter) {
    emit_icmp(emitter, LLVMIntSGT);
}

void emit_le(uniEmitter* emitter) {
    emit_icmp(emitter, LLVMIntSLE);
}

void emit_ge(uniEmitter* emitter) {
    emit_icmp(emitter, LLVMIntSGE);
}

void emit_dup(uniEmitter* emitter) {
    LLVMValueRef a = uni_emitPop(emitter);

    uni_emitPush(emitter, a);
    uni_emitPush(emitter, a);
}

void emit_drop(uniEmitter* emitter) {
    uni_emitPop(emitter);
}

void emit_swap(uniEmitter* emitter) {
    LLVMValueRef b = uni_emitPop(emitter);
    LLVMValueRef a = uni_emitPop(emitter);

    uni_emitPush(emitter, b);
    uni_emitPush(emitter, a);
}

void emit_over(uniEmitter* emitter) {
    LLVMValueRef b = uni_emitPop(emitter);
    LLVMValueRef a = uni_emitPop(emitter);

    uni_emitPush(emitter, a);
    uni_emitPush(emitter, b);
    uni_emitPush(emitter, a);
}
