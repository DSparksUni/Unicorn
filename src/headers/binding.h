#ifndef UNI_BINDING_H_INCLUDED_
#define UNI_BINDING_H_INCLUDED_

#include "word.h"

typedef struct uniBinding_t {
    const char* name;
    size_t name_len;
    uniType type;
    bool is_mut;
    bool is_global;
} uniBinding;

uniBinding* uni_lookupBinding(
    uniBinding* bindings, size_t binding_count, const char* name, size_t name_len
);

bool uni_resolveTypeName(const char* name, size_t name_len, uniType* out);

#endif
