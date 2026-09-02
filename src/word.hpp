/*
MIT License

Copyright (c) 2026 Dylan Sparks

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
        Op* body;
    };

    void registerWord(Word word);
    Word* lookupWord(std::string_view name);
}
