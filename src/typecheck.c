#include "headers/typecheck.h"
#include <stdio.h>

static char* type_name(uniType type) {
    switch(type) {
        case UNI_TYPE_INT: return "int";
        case UNI_TYPE_STRING: return "string";
    }
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

static bool tc_pop(uniTypeStack* stack, uniType expected, size_t line) {
    if(stack->count == 0) {
        fprintf(stderr, "[ERROR] (line %zu) stack underflow\n", line);
        return false;
    }

    uniType type = stack->items[--stack->count];
    if(type != expected) {
        fprintf(
            stderr, "[ERROR] (line %zu) type error: expected type %s, got %s\n",
            line, type_name(expected), type_name(type)
        );
        return false;
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

            for(size_t i = 0; i < word->num_inputs; i++) {
                if(!tc_pop(stack, word->inputs[word->num_inputs - 1 - i], op->line)) return false;
            }
            for(size_t i = 0; i < word->num_outputs; i++) {
                if(!tc_push(stack, word->outputs[i])) return false;
            }

            return true;
        };

        case UNI_OP_BLOCK: {
            return tc_block(stack, op);
        }

        case UNI_OP_IF: {
            if(!tc_pop(stack, UNI_TYPE_INT, op->line)) return false;

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
                if(then_stack.items[i] != else_stack.items[i]) {
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
