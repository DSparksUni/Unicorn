#pragma once

#include <vector>
#include <unordered_map>

#include "var.hpp"
#include "word.hpp"
#include "parser.hpp"

namespace uni {
    struct TcContext {
        std::vector<Type> stack;

        std::unordered_map<size_t, Type> bindings;
        size_t* bind_counter;

        std::vector<Variable> variables;
        bool is_global;
    };

    bool typecheck(const OpBlock* program);
}
