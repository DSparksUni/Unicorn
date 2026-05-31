#ifndef UNI_TYPECHECK_H_INCLUDED_
#define UNI_TYPECHECK_H_INCLUDED_

#include <stdlib.h>
#include <stdbool.h>

#include "parser.h"
#include "word.h"

#define UNI_MAX_VARS 24

typedef struct uniTypeStack_t {
    uniType* items;
    size_t count;
    size_t cap;
} uniTypeStack;

typedef struct uniTcContext_t {
    uniTypeStack stack;
    size_t* var_counter;
    uniType var_bindings[UNI_MAX_VARS];
    bool var_bound[UNI_MAX_VARS];
} uniTcContext;

bool uni_typecheck(uniOp* program);

#endif
