#include "parser.hpp"

#include <iostream>
#include <sstream>

namespace uni {
    Parser::Parser(std::vector<Token> tokens):
        tokens{std::move(tokens)}, cursor{0}
    {}

    Token Parser::peek() const {
        return tokens[cursor];
    }

    Token Parser::advance() {
        Token tok = tokens[cursor];
        if(tok.type != TokenType::UNI_TOKEN_EOF) cursor++;

        return tok;
    }

    bool Parser::expect(TokenType type) {
        if(peek().type == type) {
            advance();
            return true;
        }

        return false;
    }

    std::unique_ptr<Op> Parser::parseOne() {
        Token tok = peek();
        switch(tok.type) {
            case TokenType::UNI_TOKEN_INT: {
                advance();

                auto op = std::make_unique<OpPushInt>();
                op->type = OpType::UNI_OP_PUSH_INT;
                op->line = tok.line;
                op->value = std::get<int64_t>(tok.value);

                return op;
            };

            case TokenType::UNI_TOKEN_FLOAT: {
                advance();

                auto op = std::make_unique<OpPushFloat>();
                op->type = OpType::UNI_OP_PUSH_FLOAT;
                op->line = tok.line;
                op->value = std::get<double>(tok.value);

                return op;
            } break;

            case TokenType::UNI_TOKEN_STRING: {
                advance();

                auto op = std::make_unique<OpPushStr>();
                op->type = OpType::UNI_OP_PUSH_STR;
                op->line = tok.line;
                op->value = tok.text;

                return op;
            } break;

            case TokenType::UNI_TOKEN_LBRACE: {
                advance();
                return parseBlock();
            } break;

            case TokenType::UNI_TOKEN_ARROW: {
                advance();

                if(peek().type != TokenType::UNI_TOKEN_WORD) {
                    std::cout   << "[ERROR] (line " << tok.line
                                << ") '->' must be followed by a variable name\n";
                    return nullptr;
                }
                Token name_tok = advance();

                auto op = std::make_unique<OpStore>();
                op->type = OpType::UNI_OP_STORE;
                op->line = tok.line;
                op->name = tok.text;

                return op;
            } break;

            case TokenType::UNI_TOKEN_WORD: {
                advance();

                if(tok.text == "if") {
                    if(peek().type != TokenType::UNI_TOKEN_LBRACE) {
                        std::cout   << "[ERROR] (line " << tok.line
                                    << ") 'if' must be followed by a block\n";
                        return nullptr;
                    }
                    advance();

                    auto then_body = parseBlock();
                    if(!then_body) return nullptr;

                    std::unique_ptr<OpBlock> else_body = nullptr;
                    Token next = peek();
                    if(next.type == TokenType::UNI_TOKEN_WORD && next.text == "else") {
                        advance();
                        if(peek().type != TokenType::UNI_TOKEN_LBRACE) {
                            std::cout   << "[ERROR] (line " << next.line
                                        << ") 'else' must be followed by a block\n";
                            return nullptr;
                        }
                        advance();

                        else_body = parseBlock();
                        if(!else_body) return nullptr;
                    }

                    auto op = std::make_unique<OpIf>();
                    op->type = OpType::UNI_OP_IF;
                    op->line = tok.line;
                    op->then_body = std::move(then_body);
                    op->else_body = std::move(else_body);

                    return op;
                }

                if(tok.text == "while") {
                    if(peek().type != TokenType::UNI_TOKEN_LBRACE) {
                        std::cout   << "[ERROR] (line " << tok.line
                                    << ") 'while' must be followed by a condition block\n";
                        return nullptr;
                    }
                    advance();

                    auto cond_body = parseBlock();
                    if(!cond_body) return nullptr;

                    if(peek().type != TokenType::UNI_TOKEN_LBRACE) {
                        std::cout   << "[ERROR] (line " << tok.line
                                    << ") 'while' condition block must be followed by a body block\n";
                        return nullptr;
                    }
                    advance();

                    auto loop_body = parseBlock();
                    if(!loop_body) return nullptr;

                    auto op = std::make_unique<OpWhile>();
                    op->type = OpType::UNI_OP_WHILE;
                    op->line = tok.line;
                    op->cond = std::move(cond_body);
                    op->loop = std::move(loop_body);

                    return op;
                }

                if(tok.text == "def") {
                    if(peek().type != TokenType::UNI_TOKEN_WORD) {
                        std::cout   << "[ERROR] (line " << tok.line
                                    << ") 'def' must be followed by the word's name\n";
                        return nullptr;
                    }
                    Token name_tok = advance();

                    if(peek().type != TokenType::UNI_TOKEN_LBRACE) {
                        std::cout   << "[ERROR] (line " << tok.line
                                    << ") 'def' names must be followed by a block\n";
                        return nullptr;
                    }
                    advance();

                    auto body = parseBlock();
                    if(!body) return nullptr;

                    auto op = std::make_unique<OpDef>();
                    op->type = OpType::UNI_OP_DEF;
                    op->line = tok.line;
                    op->name = name_tok.text;
                    op->body = std::move(body);

                    return op;
                }

                if(tok.text == "let") {
                    bool is_mut = false;
                    if(peek().type != TokenType::UNI_TOKEN_WORD) {
                        std::cout   << "[ERROR] (line " << tok.line
                                    << ") 'let' must be followed by the variable's name\n";
                        return nullptr;
                    }

                    std::string_view name;
                    Token next = advance();
                    if(next.text == "mut") {
                        is_mut = true;
                        if(peek().type != TokenType::UNI_TOKEN_WORD) {
                            std::cout   << "[ERROR] (line " << tok.line
                                        << ") 'let' must be followed by the variable's name\n";
                            return nullptr;
                        }
                        Token name_tok = advance();
                        name = name_tok.text;
                    } else {
                        name = next.text;
                    }

                    std::string_view type_name;
                    if(peek().type != TokenType::UNI_TOKEN_COLON) {
                        std::cout   << "[ERROR] (line " << tok.line
                                    << ") Variables must be declared with an explicit type\n";
                        return nullptr;
                    } else {
                        advance();
                        Token type_name_tok = advance();
                        type_name = type_name_tok.text;
                    }

                    auto op = std::make_unique<OpLet>();
                    op->type = OpType::UNI_OP_LET;
                    op->line = tok.line;
                    op->name = name;
                    op->type_name = type_name;
                    op->is_mut = is_mut;

                    return op;
                }

                auto op = std::make_unique<OpWord>();
                op->type = OpType::UNI_OP_WORD;
                op->line = tok.line;
                op->name = tok.text;

                return op;
            } break;

            default: return nullptr;
        }
    }

    std::unique_ptr<OpBlock> Parser::parseBlock() {
        auto op = std::make_unique<OpBlock>();
        op->type = OpType::UNI_OP_BLOCK;

        while(
            peek().type != TokenType::UNI_TOKEN_LBRACE &&
            peek().type != TokenType::UNI_TOKEN_EOF
        ) {
            std::unique_ptr<Op> child = parseOne();
            if(!child) break;

            op->items.push_back(std::move(child));
        }

        expect(TokenType::UNI_TOKEN_LBRACE);
        return op;
    }

    std::unique_ptr<OpBlock> Parser::parseProgram() {
        auto op = std::make_unique<OpBlock>();
        op->type = OpType::UNI_OP_BLOCK;

        while(peek().type != TokenType::UNI_TOKEN_EOF) {
            std::unique_ptr<Op> child = parseOne();
            if(!child) break;

            op->items.push_back(std::move(child));
        }

        return op;
    }

    void add_indent(std::stringstream& ss, size_t depth) {
        for(size_t _ = 0; _ < depth; _++) ss << ' ';
    }

    void opToString_impl(const Op* op, std::stringstream& ss, size_t indent) {
        add_indent(ss, indent);
        switch(op->type) {
            case OpType::UNI_OP_PUSH_INT: {
                auto o = dynamic_cast<const OpPushInt*>(op);
                ss << "PUSH_INT " << o->value << '\n';
            } break;

            case OpType::UNI_OP_PUSH_FLOAT: {
                auto o = dynamic_cast<const OpPushFloat*>(op);
                ss << "PUSH_FLOAT " << o->value << '\n';
            } break;

            case OpType::UNI_OP_PUSH_STR: {
                auto o = dynamic_cast<const OpPushStr*>(op);
                ss << "PUSH_STR \"" << o->value << "\"\n";
            } break;

            case OpType::UNI_OP_WORD: {
                auto o = dynamic_cast<const OpWord*>(op);
                ss << "WORD \"" << o->name << "\"\n";
            } break;

            case OpType::UNI_OP_BLOCK: {
                auto o = dynamic_cast<const OpBlock*>(op);
                ss << "BLOCK (length " << o->items.size() << ")\n";
                for(const auto& sub_op : o->items)
                    opToString_impl(sub_op.get(), ss, indent+4);
            } break;

            case OpType::UNI_OP_IF: {
                auto o = dynamic_cast<const OpIf*>(op);
                ss << "IF\n";

                add_indent(ss, indent);
                ss << "    THEN:\n";
                for(const auto& sub_op : o->then_body->items)
                    opToString_impl(sub_op.get(), ss, indent+4);

                if(o->else_body) {
                    add_indent(ss, indent);
                    ss << "    ELSE:\n";
                    for(const auto& sub_op : o->else_body->items)
                        opToString_impl(sub_op.get(), ss, indent+4);
                }
            } break;

            case OpType::UNI_OP_WHILE: {
                auto o = dynamic_cast<const OpWhile*>(op);
                ss << "WHILE\n";

                add_indent(ss, indent);
                ss << "    COND:\n";
                for(const auto& sub_op : o->cond->items)
                    opToString_impl(sub_op.get(), ss, indent+4);

                add_indent(ss, indent);
                ss << "    LOOP:\n";
                for(const auto& sub_op : o->loop->items)
                    opToString_impl(sub_op.get(), ss, indent+4);
            } break;

            case OpType::UNI_OP_DEF: {
                auto o = dynamic_cast<const OpDef*>(op);
                ss << "DEF (" << o->name << ")\n";

                for(const auto& sub_op : o->body->items)
                    opToString_impl(sub_op.get(), ss, indent+4);
            } break;

            case OpType::UNI_OP_LET: {
                auto o = dynamic_cast<const OpLet*>(op);
                ss  << "LET (" << (o->is_mut? "mut " : "") << o->name
                    << "): (" << o->type_name << ")\n";
            } break;
        }
    }

    std::string opToString(const Op* op) {
        std::stringstream ss;
        opToString_impl(op, ss, 0);

        return ss.str();
    }
}
