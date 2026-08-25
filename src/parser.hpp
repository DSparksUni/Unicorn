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
        std::string_view value;

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
