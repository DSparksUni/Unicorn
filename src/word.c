#include "headers/word.h"
#include "headers/emitter.h"

void emit_add(uniEmitter* emitter);
void emit_sub(uniEmitter* emitter);
void emit_mult(uniEmitter* emitter);
void emit_div(uniEmitter* emitter);
void emit_printi(uniEmitter* emitter);
void emit_prints(uniEmitter* emitter);

static uniType INT1[] = { UNI_TYPE_INT };
static uniType INT2[] = { UNI_TYPE_INT, UNI_TYPE_INT };
static uniType STR1[] = { UNI_TYPE_STRING };

#define NUM_WORDS 6
static uniWord WORDS[NUM_WORDS] = {
    {"+", INT2, 2, INT1, 1, emit_add},
    {"-", INT2, 2, INT1, 1, emit_sub},
    {"*", INT2, 2, INT1, 1, emit_mult},
    {"/", INT2, 2, INT1, 1, emit_div},
    {"printi", INT1, 1, NULL, 0, emit_printi},
    {"prints", STR1, 1, NULL, 0, emit_prints}
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
