#include "headers/typecheck.h"
#include <stdio.h>

static char* kind_name(uniTypeKind kind) {
    switch(kind) {
        case UNI_KIND_INT: return "int";
        case UNI_KIND_STRING: return "string";
        case UNI_KIND_VAR: return "var";
    }
}
static char* type_name(uniType type) {
    return kind_name(type.kind);
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

#define MAX_VARS 24

static bool tc_apply_word(
    uniTypeStack* stack,
    uniType* inputs, size_t num_inputs,
    uniType* outputs, size_t num_outputs,
    size_t line,
    const char* word_name
) {
    if(stack->count < num_inputs) {
        fprintf(
            stderr, "[ERROR] (line %zu) '%s' needs %zu value(s) but stack only has %zu\n",
            line, word_name, num_inputs, stack->count
        );
        return false;
    }

    uniType bindings[MAX_VARS];
    bool bound[MAX_VARS];
    memset(bound, 0, sizeof(bound));

    for(size_t i = 0; i < num_inputs; i++) {
        size_t stack_idx = stack->count - 1 - i;
        uniType actual = stack->items[stack_idx];
        uniType expected = inputs[num_inputs - 1 - i];

        if(expected.kind == UNI_KIND_VAR) {
            uint8_t vid = expected.var_id;
            if(!bound[vid]) {
                bindings[vid] = actual;
                bound[vid] = true;
            } else {
                if(actual.kind != bindings[vid].kind) {
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
        } else {
            if(actual.kind != expected.kind) {
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

    stack->count -= num_inputs;

    for(size_t i = 0; i < num_outputs; i++) {
        uniType out = outputs[i];
        if(out.kind == UNI_KIND_VAR) {
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

        if(!tc_push(stack, out)) return false;
    }

    return true;
}

static bool tc_block(uniTypeStack* stack, uniOp* block);

static bool tc_op(uniTypeStack* stack, uniOp* op) {
    switch(op->type) {
        case UNI_OP_PUSH_INT: return tc_push(stack, UNI_TYPE_INT);
        case UNI_OP_PUSH_STR: return tc_push(stack, UNI_TYPE_STRING);

        case UNI_OP_WORD: {
            uniWord* word = uni_lookupWord(op->sval.start, op->sval.len);
            if(!word) {
                fprintf(
                    stderr, "[ERROR] (line %zu) Unknown word '%.*s'\n",
                    op->line, (int)op->sval.len, op->sval.start
                );
                return false;
            }

            return tc_apply_word(
                stack,
                word->inputs, word->num_inputs,
                word->outputs, word->num_outputs,
                op->line,
                word->name
            );
        };

        case UNI_OP_BLOCK: {
            return tc_block(stack, op);
        }

        case UNI_OP_IF: {
            if(stack->count == 0) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'if' needs a condition but stack is empty\n",
                    op->line
                );
                return false;
            }

            uniType cond = tc_pop_raw(stack);
            if(cond.kind != UNI_KIND_INT) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'if' condition must be int, got %s\n",
                    op->line, type_name(cond)
                );
                return false;
            }

            #define CLONE_STACK(dst, src)                                           \
                uniTypeStack dst = {0};                                             \
                dst.count = (src)->count;                                           \
                dst.cap = (src)->count;                                             \
                if(dst.cap > 0) {                                                   \
                    dst.items = malloc(dst.cap * sizeof(uniType));                  \
                    if(!dst.items) return false;                                    \
                    memcpy(dst.items, (src)->items, dst.count * sizeof(uniType));   \
                }

            CLONE_STACK(then_stack, stack);
            bool then_ok = tc_block(&then_stack, op->cval.then_body);
            if(!then_ok) {
                if(then_stack.items) free(then_stack.items);
                return false;
            }

            if(!op->cval.else_body) {
                // No else block, therefore body must be stack-neutral
                if(then_stack.count != stack->count) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) 'if' without 'else' must be stack-neutral: "
                        "started with %zu items, ended with %zu\n",
                        op->line,
                        stack->count, then_stack.count
                    );
                    free(then_stack.items);
                    return false;
                }

                free(then_stack.items);
                return true;
            }

            // Has else, therefore both branches must produce the same stack effects
            CLONE_STACK(else_stack, stack);
            bool else_ok = tc_block(&else_stack, op->cval.else_body);
            if(!else_ok) {
                if(then_stack.items) free(then_stack.items);
                if(else_stack.items) free(else_stack.items);
                return false;
            }

            if(then_stack.count != else_stack.count) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'if/else' branches produce different stack depths: "
                    "then=%zu, else=%zu\n",
                    op->line,
                    then_stack.count, else_stack.count
                );
                free(then_stack.items);
                free(else_stack.items);
                return false;
            }

            for(size_t i = 0; i < then_stack.count; i++) {
                if(then_stack.items[i].kind != else_stack.items[i].kind) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) 'if/else' branch produce different types at stack position %zu: "
                        "then=%s, else=%s\n",
                        op->line, i,
                        type_name(then_stack.items[i]), type_name(else_stack.items[i])
                    );
                    free(then_stack.items);
                    free(else_stack.items);
                    return false;
                }
            }

            // Branches match, update stack to reflect what they produced
            free(stack->items);
            stack->items = then_stack.items;
            stack->count = then_stack.count;
            stack->cap = then_stack.cap;

            free(else_stack.items);
            return true;
        }

        case UNI_OP_WHILE: {
            CLONE_STACK(cond_stack, stack);
            if(!tc_block(&cond_stack, op->wval.cond_body)) {
                free(cond_stack.items);
                return false;
            }

            if(
                cond_stack.count != stack->count + 1 ||
                cond_stack.items[cond_stack.count-1].kind != UNI_KIND_INT
            ) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'while' condition block must leave exactly one int on the stack\n",
                    op->line
                );
                free(cond_stack.items);
                return false;
            }
            free(cond_stack.items);

            CLONE_STACK(body_stack, stack);
            if(!tc_block(&body_stack, op->wval.loop_body)) {
                free(body_stack.items);
                return false;
            }

            if(body_stack.count != stack->count) {
                fprintf(
                    stderr,
                    "[ERROR] (line %zu) 'while' body must be stack-neutral\n",
                    op->line
                );
                free(body_stack.items);
                return false;
            }

            free(body_stack.items);
            return true;

            #undef CLONE_STACK
        }
    }
}

bool tc_block(uniTypeStack* stack, uniOp* block) {
    for(size_t i = 0; i < block->bval.num_items; i++) {
        if(!tc_op(stack, block->bval.items[i])) return false;
    }

    return true;
}

bool uni_typecheck(uniOp* program) {
    uniTypeStack stack = {0};

    bool result = tc_block(&stack, program);

    if(stack.items) free(stack.items);
    return result;
}
