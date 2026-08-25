#pragma once

#include <string>
#include <optional>

namespace uni {
    std::optional<std::string> readFile(const std::string& file_path);
}
