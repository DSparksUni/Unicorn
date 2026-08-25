#include "util.hpp"

#include <fstream>
#include <sstream>

namespace uni {
    std::optional<std::string> readFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if(!file) return std::nullopt;

        std::ostringstream ss;
        ss << file.rdbuf();

        return ss.str();
    }
}
