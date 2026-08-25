#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

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
        std::variant<int64_t, double> value;

        std::string toString() const;
    };

    std::vector<Token> lex(std::string_view src);
}
