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

#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <optional>

namespace uni {
    enum class TokenType {
        UNI_TOKEN_NULL,
        UNI_TOKEN_INT,
        UNI_TOKEN_FLOAT,
        UNI_TOKEN_STRING,
        UNI_TOKEN_WORD,
        UNI_TOKEN_LPAREN,
        UNI_TOKEN_RPAREN,
        UNI_TOKEN_LBRACE,
        UNI_TOKEN_RBRACE,
        UNI_TOKEN_COLON,
        UNI_TOKEN_ARROW,
        UNI_TOKEN_EOF
    };
    struct Token {
        TokenType type;
        std::string_view text;
        size_t line;
        std::variant<int64_t, double, std::string> value;

        std::string toString() const;
    };

    std::optional<std::vector<Token>> lex(std::string_view src);
}
