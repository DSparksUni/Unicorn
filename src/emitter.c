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
    emitter->str_counter = 0;
    emitter->if_counter = 0;
    emitter->stack = (uniEmitStack){NULL, 0, 0};

    return emitter;
}
void uni_destroyEmitter(uniEmitter* emitter) {
    fclose(emitter->out);
    free(emitter);
}

void uni_emitOp(uniEmitter* emitter, uniOp* op) {
    switch(op->type) {
        case UNI_OP_PUSH_INT: {
            size_t r = uni_getFreshTemp(emitter);

            fprintf(emitter->out, "    %%%zu = add i64 0, %lld\n", r, op->ival);

            uni_emitPush(emitter, r);
        } break;

        case UNI_OP_PUSH_STR: {
            size_t r = uni_getFreshTemp(emitter);
            size_t str_len = op->sval.len - 2;

            fprintf(
                emitter->out, 
                "    %%%zu = getelementptr [%zu x i8], ptr @str_%zu, i32 0, i32 0\n",
                r, str_len + 1, op->sval.global_idx
            );

            uni_emitPush(emitter, r);
        } break;

        case UNI_OP_WORD: {
            uniWord* word = uni_lookupWord(op->sval.start, op->sval.len);
            if(word) {
                // Unknown words should've been caught in typechecking
                word->emit(emitter);
            } else {
                fprintf(
                    stderr, "[ERROR] (line %zu) Unknown word '%.*s' got past typechecking\n",
                    op->line, (int)op->sval.len, op->sval.start
                );
            }
        } break;

        case UNI_OP_BLOCK: {
            // TODO: blocks
        } break;

        case UNI_OP_IF: {
            size_t id = emitter->if_counter++;
            size_t cond = uni_emitPop(emitter);
            size_t bool_tmp = uni_getFreshTemp(emitter);

            size_t stack_depth_before = emitter->stack.count;

            if(op->cval.else_body) {
                fprintf(
                    emitter->out,
                    "    %%%zu = icmp ne i64 %%%zu, 0\n"
                    "    br i1 %%%zu, label %%if_%zu_then, label %%if_%zu_else\n"
                    "\nif_%zu_then:\n",
                    bool_tmp, cond,
                    bool_tmp, id, id,
                    id
                );
            } else {
                fprintf(
                    emitter->out,
                    "    %%%zu = icmp ne i64 %%%zu, 0\n"
                    "    br i1 %%%zu, label %%if_%zu_then, label %%if_%zu_end\n"
                    "\nif_%zu_then:\n",
                    bool_tmp, cond,
                    bool_tmp, id, id,
                    id
                );
            }

            uni_emitBlock(emitter, op->cval.then_body);

            size_t num_results = emitter->stack.count - stack_depth_before;
            size_t* then_results = NULL;
            if(num_results > 0) {
                then_results = malloc(num_results * sizeof(size_t));
                if(then_results) {
                    for(size_t i = 0; i < num_results; i++) {
                        then_results[i] = emitter->stack.items[stack_depth_before + i];
                    }
                    emitter->stack.count = stack_depth_before;
                }
            }

            if(op->cval.else_body) {
                fprintf(
                    emitter->out,
                    "    br label %%if_%zu_end\n"
                    "\nif_%zu_else:\n",
                    id,
                    id
                );

                uni_emitBlock(emitter, op->cval.else_body);

                size_t* else_results = NULL;
                if(num_results > 0) {
                    else_results = malloc(num_results * sizeof(size_t));
                    for(size_t i = 0; i < num_results; i++) {
                        else_results[i] = emitter->stack.items[stack_depth_before + i];
                    }
                    emitter->stack.count = stack_depth_before;
                }

                fprintf(
                    emitter->out,
                    "    br label %%if_%zu_end\n"
                    "\nif_%zu_end:\n",
                    id,
                    id
                );

                // Emit a phi node for each value produced by both branches
                for(size_t i = 0; i < num_results; i++) {
                    size_t phi = uni_getFreshTemp(emitter);
                    fprintf(
                        emitter->out,
                        "    %%%zu = phi i64 [ %%%zu, %%if_%zu_then ], "
                        "[ %%%zu, %%if_%zu_else ]\n",
                        phi, then_results[i], id,
                        else_results[i], id
                    );

                    uni_emitPush(emitter, phi);
                }

                if(then_results) free(then_results);
                if(else_results) free(else_results);
            } else {
                fprintf(
                    emitter->out,
                    "    br label %%if_%zu_end\n"
                    "\nif_%zu_end:\n",
                    id,
                    id
                );

                if(then_results) free(then_results);
            }
        } break;
    }
}

void uni_emitBlock(uniEmitter* emitter, uniOp* block) {
    for(size_t i = 0; i < block->bval.num_items; i++) {
        uni_emitOp(emitter, block->bval.items[i]);
    }
}

static void collect_strings(uniEmitter* emitter, uniOp* op) {
    switch(op->type) {
        case UNI_OP_PUSH_STR: {
            size_t idx = emitter->str_counter++;
            op->sval.global_idx = idx;

            size_t decoded_len = 0;
            for(size_t i = 1; i < op->sval.len-1; i++) {
                if(op->sval.start[i] == '\\') i++;
                decoded_len++;
            }

            fprintf(
                emitter->out, "@str_%zu = private constant [%zu x i8] c\"",
                idx, decoded_len+1
            );
            for(size_t i = 1; i < op->sval.len-1; i++) {
                unsigned char c = (unsigned char)op->sval.start[i];
                if(c == '\\') {
                    i++;
                    switch(op->sval.start[i]) {
                        case 'n': fprintf(emitter->out, "\\0A"); break;
                        case 't': fprintf(emitter->out, "\\09"); break;
                        case 'r': fprintf(emitter->out, "\\0D"); break;
                        case '\\': fprintf(emitter->out, "\\5C"); break;
                        case '\"': fprintf(emitter->out, "\\22"); break;
                        case '0': fprintf(emitter->out, "\\00"); break;
                        default: fputc(op->sval.start[i], emitter->out); break;
                    }
                } else if(c < 32 || c > 126) {
                    fprintf(emitter->out, "\\%02X", c);
                } else {
                    fputc(c, emitter->out);
                }
            }
            fprintf(emitter->out, "\\00\"\n");
        } break;

        case UNI_OP_BLOCK: {
            for(size_t i = 0; i < op->bval.num_items; i++) {
                collect_strings(emitter, op->bval.items[i]);
            }
        } break;

        case UNI_OP_IF: {
            collect_strings(emitter, op->cval.then_body);
            if(op->cval.else_body) collect_strings(emitter, op->cval.else_body);
        } break;

        default: break;
    }
}

void uni_emitProgram(uniEmitter* emitter, uniOp* program) {
    // Module preamble
    fprintf(
        emitter->out,
        "@fmt_int = private constant [6 x i8] c\"%%lld\\0A\\00\"\n"
        "@fmt_str = private constant [3 x i8] c\"%%s\\00\"\n"
        "\n"
        "declare i32 @printf(ptr, ...)\n"
        "\n"
    );

    // Declare string constants
    collect_strings(emitter, program);

    // Emit main
    fprintf(emitter->out, "define i32 @main() {\nentry:\n");

    // Emit program body
    uni_emitBlock(emitter, program);

    fprintf(emitter->out, "    ret i32 0\n}\n");
}
