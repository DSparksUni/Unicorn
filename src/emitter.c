#include "headers/emitter.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"

uniEmitter* uni_createEmitter(void) {
    uniEmitter* emitter = malloc(sizeof(uniEmitter));
    if(!emitter) return NULL;

    emitter->ctx = LLVMContextCreate();
    emitter->module = LLVMModuleCreateWithNameInContext("uni", emitter->ctx);
    emitter->builder = LLVMCreateBuilderInContext(emitter->ctx);

    // Declare main
    LLVMTypeRef main_type = LLVMFunctionType(
        LLVMInt32TypeInContext(emitter->ctx), NULL, 0, false
    );
    emitter->func = LLVMAddFunction(emitter->module, "main", main_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(
        emitter->ctx, emitter->func, "entry"
    );
    LLVMPositionBuilderAtEnd(emitter->builder, entry);

    emitter->str_globals = NULL;
    emitter->str_count = 0;
    emitter->str_cap = 0;

    emitter->stack = (uniEmitStack){NULL, 0, 0};

    LLVMTypeRef printf_args[] = {LLVMPointerTypeInContext(emitter->ctx, 0) };
    emitter->printf_type = LLVMFunctionType(
        LLVMInt32TypeInContext(emitter->ctx),
        printf_args, 1,
        true
    );
    emitter->printf_fn = LLVMAddFunction(emitter->module, "printf", emitter->printf_type);

    return emitter;
}
void uni_destroyEmitter(uniEmitter* emitter) {
    LLVMDisposeBuilder(emitter->builder);
    LLVMDisposeModule(emitter->module);
    LLVMContextDispose(emitter->ctx);
    free(emitter->str_globals);
    free(emitter->stack.items);
    free(emitter);
}

bool uni_writeProgram(uniEmitter* emitter, const char* out_path) {
    char* err = NULL;
    if(LLVMPrintModuleToFile(emitter->module, out_path, &err)) {
        fprintf(stderr, "[ERROR] Failed to write IR: %s\n", err);
        LLVMDisposeMessage(err);
        return false;
    }
    return true;
}

void uni_emitOp(uniEmitter* emitter, uniOp* op) {
    switch(op->type) {
        case UNI_OP_PUSH_INT: {
            LLVMValueRef val = LLVMConstInt(
                LLVMInt64TypeInContext(emitter->ctx),
                op->ival,
                true
            );
            uni_emitPush(emitter, val);
        } break;

        case UNI_OP_PUSH_STR: {
            LLVMValueRef val = emitter->str_globals[op->sval.global_idx];
            LLVMValueRef zero = LLVMConstInt(
                LLVMInt64TypeInContext(emitter->ctx),
                0, false
            );
            LLVMValueRef indicies[] = {zero, zero};
            LLVMValueRef ptr = LLVMBuildGEP2(
                emitter->builder,
                LLVMGlobalGetValueType(val),
                val,
                indicies, 2,
                "strptr"
            );
            uni_emitPush(emitter, ptr);
        } break;

        case UNI_OP_WORD: {
            uniWord* word = uni_lookupWord(op->sval.start, op->sval.len);
            if(word) {
                word->emit(emitter);
            } else {
                // Unknown words should've been caught in typechecking
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
            LLVMValueRef cond = uni_emitPop(emitter);
            LLVMValueRef cond_bool = LLVMBuildICmp(
                emitter->builder,
                LLVMIntNE,
                cond,
                LLVMConstInt(LLVMInt64TypeInContext(emitter->ctx), 0, false),
                "cond"
            );

            LLVMBasicBlockRef then_block = LLVMAppendBasicBlockInContext(
                emitter->ctx, emitter->func, "then"
            );
            LLVMBasicBlockRef else_block = (op->cval.else_body) 
                ? LLVMAppendBasicBlockInContext(emitter->ctx, emitter->func, "else")
                : NULL;
            LLVMBasicBlockRef end_block = LLVMAppendBasicBlockInContext(
                emitter->ctx, emitter->func, "end"
            );

            if(op->cval.else_body) {
                LLVMBuildCondBr(emitter->builder, cond_bool, then_block, else_block);
            } else {
                LLVMBuildCondBr(emitter->builder, cond_bool, then_block, end_block);
            }

            size_t stack_depth_before = emitter->stack.count;

            LLVMPositionBuilderAtEnd(emitter->builder, then_block);
            uni_emitBlock(emitter, op->cval.then_body);
            LLVMBuildBr(emitter->builder, end_block);

            size_t num_results = emitter->stack.count - stack_depth_before;
            LLVMValueRef* then_results = NULL;
            if(num_results > 0) {
                then_results = malloc(num_results * sizeof(LLVMValueRef));
                if(then_results) {
                    for(size_t i = 0; i < num_results; i++) {
                        then_results[i] = emitter->stack.items[stack_depth_before + i];
                    }
                    emitter->stack.count = stack_depth_before;
                }
            }

            if(op->cval.else_body) {
                LLVMPositionBuilderAtEnd(emitter->builder, else_block);
                uni_emitBlock(emitter, op->cval.else_body);
                LLVMBuildBr(emitter->builder, end_block);

                LLVMValueRef* else_results = NULL;
                if(num_results > 0) {
                    else_results = malloc(num_results * sizeof(LLVMValueRef));
                    if(else_results) {
                        for(size_t i = 0; i < num_results; i++) {
                            else_results[i] = emitter->stack.items[stack_depth_before + i];
                        }
                        emitter->stack.count = stack_depth_before;
                    }
                }

                LLVMPositionBuilderAtEnd(emitter->builder, end_block);

                if(then_results && else_results) {
                    for(size_t i = 0; i < num_results; i++) {
                        LLVMValueRef phi = LLVMBuildPhi(
                            emitter->builder,
                            LLVMInt64TypeInContext(emitter->ctx),
                            "phi"
                        );
                        LLVMValueRef incoming_vals[] = {then_results[i], else_results[i]};
                        LLVMBasicBlockRef incoming_blocks[] = {then_block, else_block};
                        LLVMAddIncoming(phi, incoming_vals, incoming_blocks, 2);
                        uni_emitPush(emitter, phi);
                    }

                    free(else_results);
                }
            } else { 
                LLVMPositionBuilderAtEnd(emitter->builder, end_block);
            }

            free(then_results);
        } break;

        case UNI_OP_WHILE: {
            LLVMBasicBlockRef pre_block = LLVMGetInsertBlock(emitter->builder);
            LLVMBasicBlockRef cond_block = LLVMAppendBasicBlockInContext(
                emitter->ctx, emitter->func, "while_cond"
            );
            LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(
                emitter->ctx, emitter->func, "while_body"
            );
            LLVMBasicBlockRef end_block = LLVMAppendBasicBlockInContext(
                emitter->ctx, emitter->func, "while_end"
            );

            LLVMBuildBr(emitter->builder, cond_block);
            LLVMPositionBuilderAtEnd(emitter->builder, cond_block);

            size_t stack_base = emitter->stack.count;
            LLVMValueRef* phis = malloc(stack_base * sizeof(LLVMValueRef));
            if(!phis && stack_base > 0) return;
            for(size_t i = 0; i < stack_base; i++) {
                LLVMValueRef phi = LLVMBuildPhi(
                    emitter->builder, LLVMTypeOf(emitter->stack.items[i]), "loop-var"
                );
                LLVMAddIncoming(phi, &emitter->stack.items[i], &pre_block, 1);
                phis[i] = phi;
                emitter->stack.items[i] = phi;
            }

            uni_emitBlock(emitter, op->wval.cond_body);
            LLVMValueRef cond_val = uni_emitPop(emitter);
            LLVMValueRef cond_bool = LLVMBuildICmp(
                emitter->builder, LLVMIntNE,
                cond_val,
                LLVMConstInt(LLVMInt64TypeInContext(emitter->ctx), 0, false),
                "while_cond_bool"
            );
            LLVMBuildCondBr(emitter->builder, cond_bool, body_block, end_block);

            LLVMPositionBuilderAtEnd(emitter->builder, body_block);
            uni_emitBlock(emitter, op->wval.loop_body);

            LLVMBasicBlockRef body_end = LLVMGetInsertBlock(emitter->builder);
            for(size_t i = 0; i < stack_base; i++) {
                LLVMAddIncoming(phis[i], &emitter->stack.items[i], &body_end, 1);
            }
            LLVMBuildBr(emitter->builder, cond_block);

            free(phis);
            LLVMPositionBuilderAtEnd(emitter->builder, end_block);
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
            op->sval.global_idx = emitter->str_count;

            size_t decoded_len = 0;
            for(size_t i = 1; i < op->sval.len-1; i++) {
                if(op->sval.start[i] == '\\') i++;
                decoded_len++;
            }

            char* buffer = malloc(decoded_len + 1);
            if(!buffer) return;
            for(size_t i = 1, j = 0; i < op->sval.len-1; i++) {
                unsigned char c = (unsigned char)op->sval.start[i];
                if(c == '\\') {
                    i++;
                    switch(op->sval.start[i]) {
                        case 'n': buffer[j++] = '\n'; break;
                        case 't': buffer[j++] = '\t'; break;
                        case 'r': buffer[j++] = '\r'; break;
                        case '\\': buffer[j++] = '\\'; break;
                        case '\"': buffer[j++] = '\"'; break;
                        case '0': buffer[j++] = '\0'; break;
                        default: buffer[j++] = op->sval.start[i]; break;
                    }
                } else {
                    buffer[j++] = c;
                }
            }

            LLVMValueRef str = LLVMConstStringInContext(emitter->ctx, buffer, decoded_len, false);
            LLVMValueRef global = LLVMAddGlobal(
                emitter->module,
                LLVMArrayType(LLVMInt8TypeInContext(emitter->ctx), decoded_len+1),
                "str"
            );
            LLVMSetInitializer(global, str);
            free(buffer);


            if(emitter->str_count >= emitter->str_cap) {
                size_t new_cap = (emitter->str_cap == 0)? 8 : emitter->str_cap * 2;
                LLVMValueRef* new_strings = realloc(
                    emitter->str_globals, new_cap * sizeof(LLVMValueRef)
                );
                if(!new_strings) return;

                emitter->str_globals = new_strings;
                emitter->str_cap = new_cap;
            }
            emitter->str_globals[emitter->str_count++] = global;
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

        case UNI_OP_WHILE: {
            collect_strings(emitter, op->wval.cond_body);
            collect_strings(emitter, op->wval.loop_body);
        } break;

        default: break;
    }
}

void uni_emitProgram(uniEmitter* emitter, uniOp* program) {
    // Module preamble
    const char fmt_int[] = "%lld";
    LLVMValueRef str_fmt_int = LLVMConstStringInContext(
        emitter->ctx, fmt_int, sizeof(fmt_int), true
    );
    emitter->fmt_int = LLVMAddGlobal(
        emitter->module,
        LLVMArrayType(LLVMInt8TypeInContext(emitter->ctx), sizeof(fmt_int)),
        "fmt_int"
    );
    LLVMSetInitializer(emitter->fmt_int, str_fmt_int);

    const char fmt_str[] = "%s";
    LLVMValueRef str_fmt_str = LLVMConstStringInContext(
        emitter->ctx, fmt_str, sizeof(fmt_str), true
    );
    emitter->fmt_str = LLVMAddGlobal(
        emitter->module,
        LLVMArrayType(LLVMInt8TypeInContext(emitter->ctx), sizeof(fmt_str)),
        "fmt_str"
    );
    LLVMSetInitializer(emitter->fmt_str, str_fmt_str);

    // Declare string constants
    collect_strings(emitter, program);

    // Emit program body
    uni_emitBlock(emitter, program);

    LLVMBuildRet(
        emitter->builder,
        LLVMConstInt(LLVMInt32TypeInContext(emitter->ctx), 0, 0)
    );
}
