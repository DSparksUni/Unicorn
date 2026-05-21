#include "headers/emitter.h"

uniEmitter* uni_createEmitter(const char* out_path) {
    uniEmitter* emitter = malloc(sizeof(uniEmitter));
    if(!emitter) return NULL;

    emitter->out = fopen(out_path, "w");
    if(!emitter->out) {
        free(emitter);
        return NULL;
    }
    emitter->tmp_counter = 0;

    return emitter;
}
void uni_destroyEmitter(uniEmitter* emitter) {
    fclose(emitter->out);
    free(emitter);
}

size_t getFreshValue(uniEmitter* emitter) {
    return emitter->tmp_counter++;
}

void emitPush(
    uniEmitter* emitter,
    size_t stack, size_t sp_slot,
    const char* val_type, const char* val
) {
    size_t t0 = getFreshValue(emitter);
    size_t t1 = getFreshValue(emitter);
    size_t t2 = getFreshValue(emitter);

    fprintf(
        emitter->out,
        "    %%%zu = load i32, ptr %%%zu\n"                                     // Load sp
        "    %%%zu = getelementptr [1024 x %s], ptr %%%zu, i32 0, i32 %%%zu\n"  // Get slot
        "    store %s %s, ptr %%%zu\n"                                         // Store value
        "    %%%zu = add i32 %%%zu, 1\n"                                        // Increment sp
        "    store i32 %%%zu, ptr %%%zu\n",                                     // Write back sp
        t0, sp_slot,
        t1, val_type, stack, t0,
        val_type, val, t1,
        t2, t0,
        t2, sp_slot
    );
}

void emitPop(
    uniEmitter* emitter,
    size_t stack, size_t sp_slot,
    const char* val_type, size_t* out_tmp
) {
    size_t t0 = getFreshValue(emitter);
    size_t t1 = getFreshValue(emitter);
    size_t t2 = getFreshValue(emitter);
    size_t t3 = getFreshValue(emitter);
    *out_tmp = t3;

    fprintf(
        emitter->out,
        "    %%%zu = load i32, ptr %%%zu\n"                                     // Load sp
        "    %%%zu = sub i32 %%%zu, 1\n"                                        // Decrement sp
        "    store i32 %%%zu, ptr %%%zu\n"                                      // Write back sp
        "    %%%zu = getelementptr [1024 x %s], ptr %%%zu, i32 0, i32 %%%zu\n"  // Get slot
        "    %%%zu = load %s, ptr %%%zu\n",                                     // Load value
        t0, sp_slot,
        t1, t0,
        t1, sp_slot,
        t2, val_type, stack, t1,
        t3, val_type, t2
    );
}

void emitPushConstant(uniEmitter* emitter, size_t stack, size_t sp, int64_t ival) {
    size_t t_val = getFreshValue(emitter);
    fprintf(emitter->out, "    %%%zu = add i64 0, %lld\n", t_val, ival);

    char val[24];
    snprintf(val, sizeof(val), "%%%zu", t_val);
    emitPush(emitter, stack, sp, "i64", val);
}

void uni_emitOp(uniEmitter* emitter, uniOp* op, size_t stack, size_t sp) {
    switch(op->type) {
        case UNI_OP_PUSH_INT: {
            emitPushConstant(emitter, stack, sp, op->ival);
        } break;

        case UNI_OP_PUSH_STR: {
            // TODO: Implement push_str
        } break;

        case UNI_OP_WORD: {
            const char* word = op->sval.start;
            size_t word_len = op->sval.len;

            if(word_len == 1 && word[0] == '+') {
                size_t a, b;
                emitPop(emitter, stack, sp, "i64", &b);
                emitPop(emitter, stack, sp, "i64", &a);
                size_t r = getFreshValue(emitter);
                fprintf(emitter->out, "    %%%zu = add i64 %%%zu, %%%zu\n", r, a, b);

                char val[24];
                snprintf(val, sizeof(val), "%%%zu", r);
                emitPush(emitter, stack, sp, "i64", val);
            } else if(word_len == 1 && word[0] == '-') {
                size_t a, b;
                emitPop(emitter, stack, sp, "i64", &b);
                emitPop(emitter, stack, sp, "i64", &a);
                size_t r = getFreshValue(emitter);
                fprintf(emitter->out, "    %%%zu = sub i64 %%%zu, %%%zu\n", r, a, b);

                char val[24];
                snprintf(val, sizeof(val), "%%%zu", r);
                emitPush(emitter, stack, sp, "i64", val);
            } else if(word_len == 1 && word[0] == '*') {
                size_t a, b;
                emitPop(emitter, stack, sp, "i64", &b);
                emitPop(emitter, stack, sp, "i64", &a);
                size_t r = getFreshValue(emitter);
                fprintf(emitter->out, "    %%%zu = mul i64 %%%zu, %%%zu\n", r, a, b);

                char val[24];
                snprintf(val, sizeof(val), "%%%zu", r);
                emitPush(emitter, stack, sp, "i64", val);
            } else if(word_len == 1 && word[0] == '/') {
                size_t a, b;
                emitPop(emitter, stack, sp, "i64", &b);
                emitPop(emitter, stack, sp, "i64", &a);
                size_t r = getFreshValue(emitter);
                fprintf(emitter->out, "    %%%zu = sdiv i64 %%%zu, %%%zu\n", r, a, b);

                char val[24];
                snprintf(val, sizeof(val), "%%%zu", r);
                emitPush(emitter, stack, sp, "i64", val);
            } else if(word_len == 6 && strncmp(word, "printi", 6) == 0) {
                size_t val;
                emitPop(emitter, stack, sp, "i64", &val);
                size_t r = getFreshValue(emitter);
                fprintf(
                    emitter->out,
                    "    %%%zu = call i32 (ptr, ...) @printf(ptr @fmt_int, i64 %%%zu)\n",
                    r, val
                );
            } else if(word_len == 6 && strncmp(word, "prints", 6) == 0) {
                // TODO: prints
            }
        } break;

        case UNI_OP_BLOCK: {
            // TODO: blocks
        } break;
    }
}

void uni_emitBlock(uniEmitter* emitter, uniOp* block, size_t stack, size_t sp) {
    for(size_t i = 0; i < block->bval.num_items; i++) {
        uni_emitOp(emitter, block->bval.items[i], stack, sp);
    }
}

void uni_emitProgram(uniEmitter* emitter, uniOp* program) {
    // Module preamble
    fprintf(
        emitter->out,
        "@fmt_int = private constant [6 x i8] c\"%%lld\\0A\\00\"\n"
        "@fmt_str = private constant [4 x i8] c\"%%s\\0A\\00\"\n"
        "\n"
        "declare i32 @printf(ptr, ...)\n"
        "\n"
    );

    // Emit main
    fprintf(emitter->out, "define i32 @main() {\nentry:\n");

    // Set up stack
    size_t stack = getFreshValue(emitter);
    size_t sp = getFreshValue(emitter);
    fprintf(
        emitter->out,
        "    %%%zu = alloca [1024 x i64], align 8\n"
        "    %%%zu = alloca i32, align 4\n"
        "    store i32 0, ptr %%%zu\n",
        stack, sp, sp
    );

    // Emit program body
    uni_emitBlock(emitter, program, stack, sp);

    fprintf(emitter->out, "    ret i32 0\n}\n");
}
