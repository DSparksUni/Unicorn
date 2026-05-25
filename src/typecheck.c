#include "headers/typecheck.h"

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
