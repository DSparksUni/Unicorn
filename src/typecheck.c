#include "headers/typecheck.h"
#include <stdbool.h>

static char* kind_name(uniTypeKind kind) {
    switch(kind) {
        case UNI_KIND_INT: return "int";
        case UNI_KIND_FLOAT: return "float";
        case UNI_KIND_NUM: return "num";
        case UNI_KIND_STRING: return "string";
        case UNI_KIND_VAR: return "var";
    }
}
static char* type_name(uniType type) {
    return kind_name(type.kind);
}

static bool tc_kinds_compatible(uniTypeKind a, uniTypeKind b) {
    if(a == b) return true;
    if(a == UNI_KIND_NUM && (b == UNI_KIND_INT || b == UNI_KIND_FLOAT)) return true;
    if(b == UNI_KIND_NUM && (a == UNI_KIND_INT || a == UNI_KIND_FLOAT)) return true;

    return false;
}

static bool tc_push(uniTypeStack* stack, uniType type) {
    if(stack->count >= stack->cap) {
        size_t new_cap = (stack->cap == 0)? 8 : stack->cap * 2;
        uniType* new_items = realloc(stack->items, new_cap * sizeof(uniType));
        if(!new_items) return false;

        stack->items = new_items;
        stack->cap = new_cap;
    }

    stack->items[stack->count++] = type;
    return true;
}

static uniType tc_pop_raw(uniTypeStack* stack) {
    return stack->items[--stack->count];
}

static uniType tc_pop_underflow(uniTypeStack* stack, size_t* var_counter) {
    if(stack->count > 0) return tc_pop_raw(stack);
    else {
        return UNI_TYPE_VAR((*var_counter)++);
    }
}

static uniType tc_pop(uniTcContext* ctx) {
    if(ctx->var_counter) {
        return tc_pop_underflow(&ctx->stack, ctx->var_counter);
    }
    return tc_pop_raw(&ctx->stack);
}

static bool tc_cloneCtx(uniTcContext* src, uniTcContext* dst, size_t* var_counter) {
    *dst = (uniTcContext){0};

    dst->stack.count = src->stack.count;
    dst->stack.cap = src->stack.cap;
    if(dst->stack.cap > 0) {
        dst->stack.items = malloc(dst->stack.cap * sizeof(uniType));
        if(!dst->stack.items) return false;

        memcpy(dst->stack.items, src->stack.items, dst->stack.count * sizeof(uniType));
    }

    dst->var_counter = src->var_counter? var_counter : NULL;

    dst->num_bindings = src->num_bindings;
    dst->bindings_cap = src->bindings_cap;
    if(dst->bindings_cap > 0) {
        dst->bindings = malloc(dst->bindings_cap * sizeof(uniBinding));
        if(!dst->bindings) {
            if(dst->stack.cap > 0) free(dst->stack.items);
            return false;
        }

        memcpy(dst->bindings, src->bindings, src->num_bindings * sizeof(uniBinding));
    }

    dst->is_global = src->is_global;

    return true;
}
#define CLONE_CTX(dst, src)                                 \
    uniTcContext dst = {0};                                 \
    size_t _##dst##_local_var_counter = 0;                  \
    tc_cloneCtx(src, &dst, &_##dst##_local_var_counter);

static bool tc_apply_word(
    uniTcContext* ctx,
    uniType* inputs, size_t num_inputs,
    uniType* outputs, size_t num_outputs,
    size_t line,
    const char* word_name
) {
    if(!ctx->var_counter && ctx->stack.count < num_inputs) {
        fprintf(
            stderr, "[ERROR] (line %zu) '%s' needs %zu value(s) but stack only has %zu\n",
            line, word_name, num_inputs, ctx->stack.count
        );
        return false;
    }

    uniType bindings[UNI_MAX_VARS];
    bool bound[UNI_MAX_VARS];
    memset(bound, 0, sizeof(bound));

    size_t external_bindings = 0;
    for(size_t i = 0; i < num_inputs; i++) {
        uniType actual = tc_pop(ctx);
        uniType expected = inputs[num_inputs - 1 - i];

        if(expected.kind == UNI_KIND_VAR) {
            uint8_t vid = expected.var_id;
            if(!bound[vid]) {
                bindings[vid] = actual;
                bound[vid] = true;
            } else {
                if(!tc_kinds_compatible(actual.kind, bindings[vid].kind)) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) '%s' type mismatch: "
                        "type variable '%c' was bound to %s but got %s\n",
                        line, word_name,
                        'A' + vid, type_name(bindings[vid]), type_name(actual)
                    );
                    return false;
                }
            }
        } else if(actual.kind == UNI_KIND_VAR) {
            uint8_t vid = actual.var_id;
            if(!bound[vid] && expected.kind != UNI_KIND_VAR) {
                bindings[vid] = expected;
                ctx->var_bindings[external_bindings] = expected;

                bound[vid] = true;
                ctx->var_bound[external_bindings] = true;

                external_bindings++;
            } else {
                if(!tc_kinds_compatible(expected.kind, bindings[vid].kind)) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) '%s' type mismatch: "
                        "expected %s but got %s\n",
                        line, word_name,
                        type_name(expected), type_name(bindings[vid])
                    );
                    return false;
                }
            }
        } else if(expected.kind == UNI_KIND_NUM) {
            uint8_t nid = expected.var_id;
            if(!tc_kinds_compatible(actual.kind, UNI_KIND_NUM)) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) '%s' type mismatch: "
                    "expected number but got %s\n",
                    line, word_name,
                    type_name(actual)
                );
                return false;
            }

            if(!bound[nid]) {
                bindings[nid] = actual;
                bound[nid] = true;
            } else if(actual.kind == UNI_KIND_INT || actual.kind == UNI_KIND_FLOAT) {
                if(actual.kind == UNI_KIND_FLOAT) bindings[nid] = actual;
            } else {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) '%s' type mismatch: "
                    "type variable '%c' was bound to %s but got %s\n",
                    line, word_name,
                    'A' + nid, type_name(bindings[nid]), type_name(actual)
                );
                return false;
            }
        } else {
            if(!tc_kinds_compatible(actual.kind, expected.kind)) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) '%s' type mismatch: "
                    "expected %s but got %s\n",
                    line, word_name,
                    type_name(expected), type_name(actual)
                );
                return false;
            }
        }
    }

    for(size_t i = 0; i < num_outputs; i++) {
        uniType out = outputs[i];
        if(out.kind == UNI_KIND_VAR || out.kind == UNI_KIND_NUM) {
            uint8_t vid = out.var_id;
            if(!bound[vid]) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) '%s' output type variable '%c' is unbound "
                    "(word definition error)",
                    line, word_name, 'A' + vid
                );
                return false;
            }

            out = bindings[vid];
        }

        if(!tc_push(&ctx->stack, out)) return false;
    }

    return true;
}

static bool tc_block(uniTcContext* ctx, uniOp* block);

static bool tc_op(uniTcContext* ctx, uniOp* op) {
    switch(op->type) {
        case UNI_OP_PUSH_INT: return tc_push(&ctx->stack, UNI_TYPE_INT);
        case UNI_OP_PUSH_FLOAT: return tc_push(&ctx->stack, UNI_TYPE_FLOAT);
        case UNI_OP_PUSH_STR: return tc_push(&ctx->stack, UNI_TYPE_STRING);

        case UNI_OP_WORD: {
            uniBinding* bind = uni_lookupBinding(
                ctx->bindings, ctx->num_bindings, op->sval.start, op->sval.len
            );
            if(bind) {
                tc_push(&ctx->stack, bind->type);
                return true;
            }

            uniWord* word = uni_lookupWord(op->sval.start, op->sval.len);
            if(!word) {
                fprintf(
                    stderr, "[ERROR] (line %zu) Unknown word '%.*s'\n",
                    op->line, (int)op->sval.len, op->sval.start
                );
                return false;
            }

            return tc_apply_word(
                ctx,
                word->inputs, word->num_inputs,
                word->outputs, word->num_outputs,
                op->line,
                word->name
            );
        };

        case UNI_OP_BLOCK: {
            return tc_block(ctx, op);
        }

        case UNI_OP_IF: {
            bool status = true;

            // Check if the condition is on the stack
            if(!ctx->var_counter && ctx->stack.count == 0) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'if' needs a condition but stack is empty\n",
                    op->line
                );
                return false;
            }

            // Check if the condition is an int
            uniType cond = tc_pop(ctx);
            if(cond.kind != UNI_KIND_INT) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'if' condition must be int, got %s\n",
                    op->line, type_name(cond)
                );
                return false;
            }

            // Check the then block into its own context
            CLONE_CTX(then_ctx, ctx);
            bool then_ok = tc_block(&then_ctx, op->cval.then_body);
            if(!then_ok) {
                status = false;
                goto free_then_ctx_items;
            }

            // If there is no else block, then the if body must be stack-neutral
            if(!op->cval.else_body) {
                if(!ctx->var_counter && then_ctx.stack.count != ctx->stack.count) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) 'if' without 'else' must be stack-neutral: "
                        "started with %zu items, ended with %zu\n",
                        op->line,
                        ctx->stack.count, then_ctx.stack.count
                    );
                    status = false;
                    goto free_then_ctx_bindings;
                }

                goto free_then_ctx_bindings;
            }

            // Check the else block into its own context
            CLONE_CTX(else_ctx, ctx);
            bool else_ok = tc_block(&else_ctx, op->cval.else_body);
            if(!else_ok) {
                status = false;
                goto free_else_ctx_bindings;
            }

            // Ensure both the then and else blocks result in the same stack depth
            if(then_ctx.stack.count != else_ctx.stack.count) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'if/else' branches produce different stack depths: "
                    "then=%zu, else=%zu\n",
                    op->line,
                    then_ctx.stack.count, else_ctx.stack.count
                );
                status = false;
                goto free_else_ctx_bindings;
            }

            // Ensure both blocks produce the same types in the same position
            for(size_t i = 0; i < then_ctx.stack.count; i++) {
                if(then_ctx.stack.items[i].kind != else_ctx.stack.items[i].kind) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) 'if/else' branch produce different types at stack position %zu: "
                        "then=%s, else=%s\n",
                        op->line, i,
                        type_name(then_ctx.stack.items[i]), type_name(else_ctx.stack.items[i])
                    );
                    status = false;
                    goto free_else_ctx_bindings;
                }
            }

            // Update the stack to reflect what the blocks produce
            free(ctx->stack.items);
            ctx->stack.items = then_ctx.stack.items;
            then_ctx.stack.items = NULL;
            ctx->stack.count = then_ctx.stack.count;
            ctx->stack.cap = then_ctx.stack.cap;

        free_else_ctx_bindings:
            free(else_ctx.bindings);
        free_else_ctx_items:
            free(else_ctx.stack.items);
        free_then_ctx_bindings:
            free(then_ctx.bindings);
        free_then_ctx_items:
            free(then_ctx.stack.items);

            return status;
        }

        case UNI_OP_WHILE: {
            bool status = true;

            // Check the cond block into its own context
            CLONE_CTX(cond_ctx, ctx);
            if(!tc_block(&cond_ctx, op->wval.cond_body)) {
                status = false;
                goto free_cond_ctx_bindings;
            }

            // Check both that the condition context has the condition on the stack,
            // and the condition is an int

            // Normal case: no var counter, therefore not in inference mode
            if(!ctx->var_counter && (
                cond_ctx.stack.count != ctx->stack.count + 1 ||
                cond_ctx.stack.items[cond_ctx.stack.count-1].kind != UNI_KIND_INT
            )) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'while' condition block must leave exactly one int on the stack\n",
                    op->line
                );
                status = false;
                goto free_cond_ctx_bindings;
            }

            // Inference case: there is a var counter, therefore in inference mode
            if(
                ctx->var_counter &&
                cond_ctx.stack.count > 0 &&
                cond_ctx.stack.items[cond_ctx.stack.count-1].kind != UNI_KIND_INT
            ) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'while' condition block must leave exactly one int on the stack\n",
                    op->line
                );
                status = false;
                goto free_cond_ctx_bindings;
            }

            // Check the body clock into its own context
            CLONE_CTX(body_ctx, ctx);
            if(!tc_block(&body_ctx, op->wval.loop_body)) {
                status = false;
                goto free_body_ctx_bindings;
            }

            if(!ctx->var_counter && body_ctx.stack.count != ctx->stack.count) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'while' body must be stack-neutral\n",
                    op->line
                );
                status = false;
            }

        free_body_ctx_bindings:
            free(body_ctx.bindings);
        free_body_ctx_items:
            free(body_ctx.stack.items);
        free_cond_ctx_bindings:
            free(cond_ctx.bindings);
        free_cond_ctx_items:
            free(cond_ctx.stack.items);

            return status;
        }

        case UNI_OP_DEF: {
            bool status = true;

            // Check is the var counter is non-null, which would mean the typechecker is in inference mode,
            // which is currently only set by word definitions, because nested words are currently
            // not allowed
            if(ctx->var_counter) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) Nested word definitions are not allowed\n",
                    op->line
                );
                return false;
            }

            // Copy the name of the word definition out of the op
            char* name = malloc(op->dval.name_len + 1);
            if(!name) return false;
            memcpy(name, op->dval.name, op->dval.name_len);
            name[op->dval.name_len] = '\0';

            // Check if the name is already a defined word
            if(uni_lookupWord(name, op->dval.name_len)) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) Duplicate word '%s'\n",
                    op->line, name
                );
                status = false;
                goto free_name;
            }

            // First pass: create an inference context with an empty stack
            size_t var_counter = 0;
            uniTcContext inf_ctx = {
                .stack = {0},
                .var_counter = &var_counter,
                .num_bindings = ctx->num_bindings,
                .bindings_cap = ctx->bindings_cap
            };
            if(ctx->num_bindings > 0) {
                inf_ctx.bindings = malloc(ctx->bindings_cap * sizeof(uniBinding));
                if(!inf_ctx.bindings) {
                    status = false;
                    goto free_name;
                }
                memcpy(inf_ctx.bindings, ctx->bindings, ctx->num_bindings * sizeof(uniBinding));
            }
            if(!tc_block(&inf_ctx, op->dval.body)) {
                status = false;
                goto free_inf_ctx_bindings;
            }

            // Second pass: create a normal checking context with an empty stack
            uniTcContext check_ctx = {
                .stack = {0},
                .var_counter = NULL,
                .num_bindings = ctx->num_bindings,
                .bindings_cap = ctx->bindings_cap
            };
            if(ctx->num_bindings > 0) {
                check_ctx.bindings = malloc(ctx->bindings_cap * sizeof(uniBinding));
                if(!check_ctx.bindings) {
                    status = false;
                    goto free_inf_ctx_bindings;
                }
                memcpy(check_ctx.bindings, ctx->bindings, ctx->num_bindings * sizeof(uniBinding));
            }

            for(size_t i = 0; i < var_counter; i++)
                tc_push(&check_ctx.stack, UNI_TYPE_VAR(i));
            if(!tc_block(&check_ctx, op->dval.body)) {
                status = false;
                goto free_check_ctx_bindings;
            }

            uniType* inputs = malloc(var_counter * sizeof(uniType));
            if(!inputs && var_counter > 0) {
                status = false;
                goto free_check_ctx_bindings;
            }
            for(size_t i = 0; i < var_counter; i++) {
                inputs[i] = check_ctx.var_bound[i]? check_ctx.var_bindings[i] : UNI_TYPE_VAR(i);
            }

            uniType* outputs = malloc(check_ctx.stack.count * sizeof(uniType));
            if(!outputs && check_ctx.stack.count > 0) {
                status = false;
                goto free_check_ctx_bindings;
            }
            for(size_t i = 0; i < check_ctx.stack.count; i++) {
                uniType t = check_ctx.stack.items[i];
                if(
                    (t.kind == UNI_KIND_VAR || t.kind == UNI_KIND_NUM) &&
                    check_ctx.var_bound[t.var_id]
                ) {
                    t = check_ctx.var_bindings[t.var_id];
                }

                outputs[i] = t;
            }

            uniWord word = {
                name,
                inputs, var_counter,
                outputs, check_ctx.stack.count,
                NULL
            };
            word.body = op->dval.body;
            uni_registerWord(word);
            name = NULL;

        free_check_ctx_bindings:
            free(check_ctx.bindings);
        free_check_ctx_items:
            free(check_ctx.stack.items);
        free_inf_ctx_bindings:
            free(inf_ctx.bindings);
        free_inf_ctx_items:
            free(inf_ctx.stack.items);
        free_name:
            free(name);

            return status;
        } break;

        case UNI_OP_LET: {
            if(ctx->is_global) {
                uniType bind_type;
                if(!uni_resolveTypeName(op->lval.type_name, op->lval.type_name_len, &bind_type)) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) Unknown type name for variable: '%.*s'\n",
                        op->line, (int)op->lval.type_name_len, op->lval.type_name
                    );
                    return false;
                }

                if(ctx->num_bindings >= ctx->bindings_cap) {
                    size_t new_cap = (ctx->bindings_cap == 0)? 8 : ctx->bindings_cap * 2;
                    uniBinding* new_bindings = realloc(ctx->bindings, new_cap * sizeof(uniBinding));
                    if(!new_bindings) return false;

                    ctx->bindings = new_bindings;
                    ctx->bindings_cap = new_cap;
                }

                ctx->bindings[ctx->num_bindings++] = (uniBinding){
                    .name = op->lval.name,
                    .name_len = op->lval.name_len,
                    .type = bind_type,
                    .is_mut = op->lval.is_mut,
                    .is_global = ctx->is_global
                };

                return true;
            } else {
                // TODO: For now, all bindings will be global until functions are defined
                return false;
            }
        }

        case UNI_OP_STORE: {
            uniBinding* bind = uni_lookupBinding(
                ctx->bindings, ctx->num_bindings, op->stval.name, op->stval.name_len
            );
            if(!bind) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) Unknown variable '%.*s'\n",
                    op->line, (int)op->stval.name_len, op->stval.name
                );
                return false;
            }

            if(!bind->is_mut) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) Cannot store into unmutable variable '%.*s'\n",
                    op->line, (int)op->stval.name_len, op->stval.name
                );
                return false;
            }

            if(ctx->stack.count == 0) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'store' requires a value on the stack\n",
                    op->line
                );
                return false;
            }

            uniType actual = tc_pop(ctx);
            if(actual.kind != bind->type.kind) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'store' type mismatch: "
                    "expected %s but got %s\n",
                    op->line,
                    type_name(bind->type), type_name(actual)
                );
                return false;
            }

            return true;
        }
    }
}

bool tc_block(uniTcContext* ctx, uniOp* block) {
    for(size_t i = 0; i < block->bval.num_items; i++) {
        if(!tc_op(ctx, block->bval.items[i])) return false;
    }

    return true;
}

bool uni_typecheck(uniOp* program) {
    uniTypeStack stack = {0};
    uniTcContext ctx = {
        .stack = stack,
        .var_counter = NULL,
        .bindings = NULL,
        .is_global = true
    };

    bool result = tc_block(&ctx, program);

    if(ctx.stack.items) free(ctx.stack.items);
    if(ctx.bindings) free(ctx.bindings);
    return result;
}

#undef CLONE_CTX
