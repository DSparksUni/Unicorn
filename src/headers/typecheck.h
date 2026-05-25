#ifndef UNI_TYPECHECK_H_INCLUDED_
#define UNI_TYPECHECK_H_INCLUDED_

#include <stdlib.h>
#include <stdbool.h>

#include "parser.h"
#include "word.h"

typedef struct uniTypeStack_t {
    uniType* items;
    size_t count;
    size_t cap;
} uniTypeStack;

bool uni_typecheck(uniOp* program);

#endif
