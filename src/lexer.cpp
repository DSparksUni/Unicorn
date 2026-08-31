#include "lexer.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <charconv>

#include <fast_float/fast_float.h>
#include <unordered_map>

std::optional<std::string> decodeEscapes(std::string_view raw);

namespace uni {
    std::optional<std::vector<Token>> lex(std::string_view src) {
        std::vector<Token> tokens;
        size_t pos = 0;
        size_t line = 1;

        auto peek = [&](size_t offset = 0) -> char {
            return (pos + offset < src.size())? src[pos+offset] : '\0';
        };

        while(true) {
            while(pos < src.size() && (
                peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n'
            )) {
                if(peek() == '\n') line++;
                pos++;
            }

            if(pos >= src.size()) {
                tokens.push_back({TokenType::UNI_TOKEN_EOF, {}, line});
                break;
            }

            size_t start = pos;
            char c = peek();

            switch(c) {
                case '(': {
                    tokens.push_back({TokenType::UNI_TOKEN_LPAREN, src.substr(pos, 1), line});
                    pos++;
                } break;

                case ')': {
                    tokens.push_back({TokenType::UNI_TOKEN_RPAREN, src.substr(pos, 1), line});
                    pos++;
                } break;

                case '{': {
                    tokens.push_back({TokenType::UNI_TOKEN_LBRACE, src.substr(pos, 1), line});
                    pos++;
                } break;

                case '}': {
                    tokens.push_back({TokenType::UNI_TOKEN_RBRACE, src.substr(pos, 1), line});
                    pos++;
                } break;

                case ':': {
                    tokens.push_back({TokenType::UNI_TOKEN_COLON, src.substr(pos, 1), line});
                    pos++;
                } break;

                case '\"': {
                    pos++;  // Skip opening quote
                    while(pos < src.size() && peek() != '\"') {
                        if(peek() == '\\') pos++;
                        pos++;
                    }
                    if(pos < src.size()) pos++; // Skip closing quote

                    std::string_view raw = src.substr(start+1, pos-start-2);
                    auto esc_result = decodeEscapes(raw);
                    if(!esc_result) {
                        std::cerr   << "[ERROR] (line " << line
                                    << ") Invalid escape sequence\n";
                        return std::nullopt;
                    }

                    tokens.push_back({
                        TokenType::UNI_TOKEN_STRING,
                        src.substr(start+1, pos-start-2),
                        line,
                        esc_result.value()
                    });
                } break;

                default: {
                    if(
                        std::isdigit(static_cast<unsigned char>(c)) ||
                        (c == '-' && std::isdigit(static_cast<unsigned char>(peek(1))))
                    ) {
                        int64_t ival;
                        double fval;
                        auto int_result = std::from_chars(
                            src.data()+pos, src.data()+src.size(), ival
                        );
                        auto flt_result = fast_float::from_chars(
                            src.data()+pos, src.data()+src.size(), fval
                        );

                        const char* win_ptr;
                        TokenType type;
                        std::variant<int64_t, double, std::string> val;
                        if(int_result.ec == std::errc{} && flt_result.ptr == int_result.ptr) {
                            type = TokenType::UNI_TOKEN_INT;
                            win_ptr = int_result.ptr;
                            val = ival;
                        } else {
                            type = TokenType::UNI_TOKEN_FLOAT;
                            win_ptr = flt_result.ptr;
                            val = fval;
                        }

                        size_t consumed = win_ptr - (src.data()+pos);
                        std::string_view text = src.substr(pos, consumed);

                        tokens.push_back({type, text, line, val});
                        pos += consumed;
                    } else if(c == '-' && peek(1) == '>') {
                        tokens.push_back({TokenType::UNI_TOKEN_ARROW, src.substr(pos, 2), line});
                        pos += 2;
                    } else {
                        while(pos < src.size() && (
                            peek() != ' ' && peek() != '\t' && peek() != '\r' && peek() != '\n' &&
                            peek() != '(' && peek() != ')' && peek() != '{' && peek() != '}' &&
                            peek() != '\"' && peek() != ':'
                        )) pos++;

                        tokens.push_back({TokenType::UNI_TOKEN_WORD, src.substr(start, pos-start), line});
                    }
                } break;
            }
        }

        return tokens;
    }

    std::string Token::toString() const {
        std::stringstream str;

        str << "TOKEN(";

        str << "    type= ";
        switch(type) {
            case TokenType::UNI_TOKEN_NULL:    str << "NULL\n";   break;
            case TokenType::UNI_TOKEN_INT:     str << "INT\n";    break;
            case TokenType::UNI_TOKEN_FLOAT:   str << "FLOAT\n";  break;
            case TokenType::UNI_TOKEN_STRING:  str << "STRING\n"; break;
            case TokenType::UNI_TOKEN_WORD:    str << "WORD\n";   break;
            case TokenType::UNI_TOKEN_LPAREN:  str << "LPAREN\n"; break;
            case TokenType::UNI_TOKEN_RPAREN:  str << "RPAREN\n"; break;
            case TokenType::UNI_TOKEN_LBRACE:  str << "LBRACE\n"; break;
            case TokenType::UNI_TOKEN_RBRACE:  str << "RBRACE\n"; break;
            case TokenType::UNI_TOKEN_COLON:   str << "COLON\n";  break;
            case TokenType::UNI_TOKEN_ARROW:   str << "ARROW\n";  break;
            case TokenType::UNI_TOKEN_EOF:     str << "EOF\n";    break;
        }

        str << "    val = " << text << '\n';

        str << ")\n";
        return str.str();
    }
}

std::optional<std::string> decodeEscapes(std::string_view raw) {
    static const std::unordered_map<char, char> escapes = {
        {'n', '\n'}, {'t', '\t'}, {'r', '\r'},
        {'\\', '\\'}, {'\"', '\"'}, {'0', '\0'}
    };

    std::string out;
    out.reserve(raw.size());

    for(size_t i = 0; i < raw.size(); i++) {
        if(raw[i] != '\\') {
            out += raw[i];
            continue;
        }

        if(i+1 >= raw.size()) return std::nullopt;

        auto it = escapes.find(raw[i+1]);
        if(it == escapes.end()) return std::nullopt;

        out += it->second;
        i++;
    }

    return out;
}

