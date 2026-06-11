#include "headers/binding.h"

uniBinding* uni_lookupBinding(
    uniBinding* bindings, size_t binding_count, const char* name, size_t name_len
) {
    for(size_t i = binding_count; i > 0; i--) {
        if(
            bindings[i-1].name_len == name_len &&
            strncmp(name, bindings[i-1].name, name_len
        ) == 0) {
            return &bindings[i-1];
        }
    }

    return NULL;
}

bool uni_resolveTypeName(const char* name, size_t name_len, uniType* out) {
    if(name_len == 3 && strncmp(name, "int", 3) == 0) {
        *out = UNI_TYPE_INT;
        return true;
    } else if(name_len == 5 && strncmp(name, "float", 5) == 0) {
        *out = UNI_TYPE_FLOAT;
        return true;
    } else if(name_len == 6 && strncmp(name, "string", 6) == 0) {
        *out = UNI_TYPE_STRING;
        return true;
    }

    return false;
}
