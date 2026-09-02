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

#include <memory>

#include "lexer.hpp"

namespace uni {
    enum class OpType {
        UNI_OP_PUSH_INT,
        UNI_OP_PUSH_FLOAT,
        UNI_OP_PUSH_STR,
        UNI_OP_WORD,
        UNI_OP_BLOCK,
        UNI_OP_IF,
        UNI_OP_WHILE,
        UNI_OP_DEF,
        UNI_OP_LET,
        UNI_OP_STORE
    };

    struct Op {
        OpType type;
        size_t line;

        virtual ~Op() = default;
    };

    struct OpPushInt : Op {
        int64_t value;

        virtual ~OpPushInt() = default;
    };

    struct OpPushFloat : Op {
        double value;

        virtual ~OpPushFloat() = default;
    };

    struct OpPushStr : Op {
        std::string value;
        size_t global_idx = 0;

        virtual ~OpPushStr() = default;
    };

    struct OpWord : Op {
        std::string_view name;

        virtual ~OpWord() = default;
    };

    struct OpBlock : Op {
        std::vector<std::unique_ptr<Op>> items;

        virtual ~OpBlock() = default;
    };

    struct OpIf : Op {
        std::unique_ptr<OpBlock> then_body;
        std::unique_ptr<OpBlock> else_body;

        virtual ~OpIf() = default;
   };

    struct OpWhile : Op {
        std::unique_ptr<OpBlock> cond;
        std::unique_ptr<OpBlock> loop;

        virtual ~OpWhile() = default;
    };

    struct OpDef : Op {
        std::string_view name;
        std::unique_ptr<OpBlock> body;

        virtual ~OpDef() = default;
    };

    struct OpLet : Op {
        std::string_view name;
        std::string_view type_name;
        bool is_mut;

        virtual ~OpLet() = default;
    };

    struct OpStore : Op {
        std::string_view name;

        virtual ~OpStore() = default;
    };

    class Parser {
    public:
        Parser(std::vector<Token> tokens);

        Token peek() const;
        Token advance();
        bool expect(TokenType type);

        std::unique_ptr<Op> parseOne();
        std::unique_ptr<OpBlock> parseBlock();
        std::unique_ptr<OpBlock> parseProgram();

    private:
        std::vector<Token> tokens;
        size_t cursor;
    };

    std::string opToString(const Op* op);
}
