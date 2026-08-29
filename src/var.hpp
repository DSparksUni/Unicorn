#pragma once

#include <string_view>

#include "word.hpp"

namespace uni {
    struct Variable {
        std::string_view name;
        Type type;
        bool is_mut;
        bool is_global;
    };
}
