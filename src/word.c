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

static uniType INT1[] = { UNI_TYPE_INT };
static uniType INT2[] = { UNI_TYPE_INT, UNI_TYPE_INT };
static uniType STR1[] = { UNI_TYPE_STRING };

#define NUM_WORDS 12
static uniWord WORDS[NUM_WORDS] = {
    {"+", INT2, 2, INT1, 1, emit_add},
    {"-", INT2, 2, INT1, 1, emit_sub},
    {"*", INT2, 2, INT1, 1, emit_mult},
    {"/", INT2, 2, INT1, 1, emit_div},
    {"printi", INT1, 1, NULL, 0, emit_printi},
    {"prints", STR1, 1, NULL, 0, emit_prints},
    {"==", INT2, 2, INT1, 1, emit_eq},
    {"!=", INT2, 2, INT1, 1, emit_neq},
    {"<", INT2, 2, INT1, 1, emit_lt},
    {">", INT2, 2, INT1, 1, emit_gt},
    {"<=", INT2, 2, INT1, 1, emit_le},
    {">=", INT2, 2, INT1, 1, emit_ge},
};

uniWord* uni_lookupWord(const char* name, size_t len) {
    for(size_t i = 0; i < NUM_WORDS; i++) {
        if(
            strlen(WORDS[i].name) == len &&
            strncmp(WORDS[i].name, name, len) == 0
        ) return &WORDS[i];
    }

    return NULL;
}

void emit_add(uniEmitter* emitter) {
    size_t b = uni_emitPop(emitter);
    size_t a = uni_emitPop(emitter);
    size_t r = uni_getFreshTemp(emitter);

    fprintf(emitter->out, "    %%%zu = add i64 %%%zu, %%%zu\n", r, a, b);

    uni_emitPush(emitter, r);
}

void emit_sub(uniEmitter* emitter) {
    size_t b = uni_emitPop(emitter);
    size_t a = uni_emitPop(emitter);
    size_t r = uni_getFreshTemp(emitter);

    fprintf(emitter->out, "    %%%zu = sub i64 %%%zu, %%%zu\n", r, a, b);

    uni_emitPush(emitter, r);
}

void emit_mult(uniEmitter* emitter) {
    size_t b = uni_emitPop(emitter);
    size_t a = uni_emitPop(emitter);
    size_t r = uni_getFreshTemp(emitter);

    fprintf(emitter->out, "    %%%zu = mul i64 %%%zu, %%%zu\n", r, a, b);

    uni_emitPush(emitter, r);
}

void emit_div(uniEmitter* emitter) {
    size_t b = uni_emitPop(emitter);
    size_t a = uni_emitPop(emitter);
    size_t r = uni_getFreshTemp(emitter);

    fprintf(emitter->out, "    %%%zu = sdiv i64 %%%zu, %%%zu\n", r, a, b);

    uni_emitPush(emitter, r);
}

void emit_printi(uniEmitter* emitter) {
    size_t a = uni_emitPop(emitter);
    size_t r = uni_getFreshTemp(emitter);

    fprintf(
        emitter->out, "    %%%zu = call i32 (ptr, ...) @printf(ptr @fmt_int, i64 %%%zu)\n",
        r, a
    );
}

void emit_prints(uniEmitter* emitter) {
    size_t a = uni_emitPop(emitter);
    size_t r = uni_getFreshTemp(emitter);

    fprintf(
        emitter->out, "    %%%zu = call i32 (ptr, ...) @printf(ptr @fmt_str, ptr %%%zu)\n",
        r, a
    );
}

static void emit_icmp(uniEmitter* emitter, const char* pred) {
    size_t b = uni_emitPop(emitter);
    size_t a = uni_emitPop(emitter);
    size_t cmp = uni_getFreshTemp(emitter);
    size_t r = uni_getFreshTemp(emitter);

    fprintf(
        emitter->out,
        "    %%%zu = icmp %s i64 %%%zu, %%%zu\n"
        "    %%%zu = zext i1 %%%zu to i64\n",
        cmp, pred, a, b,
        r, cmp
    );

    uni_emitPush(emitter, r);
}

void emit_eq(uniEmitter* emitter) {
    emit_icmp(emitter, "eq");
}

void emit_neq(uniEmitter* emitter) {
    emit_icmp(emitter, "ne");
}

void emit_lt(uniEmitter* emitter) {
    emit_icmp(emitter, "slt");
}

void emit_gt(uniEmitter* emitter) {
    emit_icmp(emitter, "sgt");
}

void emit_le(uniEmitter* emitter) {
    emit_icmp(emitter, "sle");
}

void emit_ge(uniEmitter* emitter) {
    emit_icmp(emitter, "sge");
}
