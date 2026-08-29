#pragma once

#include "parser.hpp"

namespace uni {
    struct Emitter;

    enum class TypeKind {
        UNI_KIND_INT,
        UNI_KIND_FLOAT,
        UNI_KIND_NUM,
        UNI_KIND_STRING,
        UNI_KIND_VAR
    };

    struct Type {
        TypeKind kind;
        size_t bind_id;
    };

    #define UNI_TYPE_INT (uni::Type{uni::TypeKind::UNI_KIND_INT, 0})
    #define UNI_TYPE_FLOAT (uni::Type{uni::TypeKind::UNI_KIND_FLOAT, 0})
    #define UNI_TYPE_NUM(id) (uni::Type{uni::TypeKind::UNI_KIND_NUM, (id)})
    #define UNI_TYPE_STRING (uni::Type{uni::TypeKind::UNI_KIND_STRING, 0})
    #define UNI_TYPE_VAR(id) (uni::Type{uni::TypeKind::UNI_KIND_VAR, (id)})

    struct Word {
        std::string_view name;
        std::vector<Type> inputs;
        std::vector<Type> outputs;
        void (*emit)(Emitter& emitter);
        const Op* body;
    };

    void registerWord(Word word);
    Word* lookupWord(std::string_view name);
}
