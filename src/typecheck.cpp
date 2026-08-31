#include "typecheck.hpp"
#include "word.hpp"

#include <algorithm>
#include <iostream>

static std::string type_name(uni::TypeKind kind);
static bool tc_kinds_compatible(uni::TypeKind a, uni::TypeKind b);

static void tc_push(std::vector<uni::Type>& stack, uni::Type type);
static uni::Type tc_pop_raw(std::vector<uni::Type>& stack);
static uni::Type tc_pop_underflow(std::vector<uni::Type>& stack, size_t* bind_counter);
static uni::Type tc_pop(uni::TcContext& ctx);

static void tc_cloneCtx(uni::TcContext& ctx, uni::TcContext& dst, size_t* bind_counter);

static bool tc_block(uni::TcContext& ctx, const uni::OpBlock* block);
static bool tc_op(uni::TcContext& ctx, const uni::Op* op);

namespace uni {
    bool typecheck(const OpBlock* program) {
        TcContext ctx{
            .bind_counter = nullptr,
            .is_global = true,
        };

        return tc_block(ctx, program);
    }
}

static std::string type_name(uni::TypeKind kind) {
    switch(kind) {
        case uni::TypeKind::UNI_KIND_INT: return "int";
        case uni::TypeKind::UNI_KIND_FLOAT: return "float";
        case uni::TypeKind::UNI_KIND_NUM: return "num";
        case uni::TypeKind::UNI_KIND_STRING: return "string";
        case uni::TypeKind::UNI_KIND_VAR: return "var";
    }
}

static bool tc_kinds_compatible(uni::TypeKind a, uni::TypeKind b) {
    if(a == b) return true;
    if(
        a == uni::TypeKind::UNI_KIND_NUM &&
        (b == uni::TypeKind::UNI_KIND_INT || b == uni::TypeKind::UNI_KIND_FLOAT)
    ) return true;
    if(
        b == uni::TypeKind::UNI_KIND_NUM &&
        (a == uni::TypeKind::UNI_KIND_INT || a == uni::TypeKind::UNI_KIND_FLOAT)
    ) return true;

    return false;
}

static void tc_push(std::vector<uni::Type>& stack, uni::Type type) {
    stack.push_back(type);
}

static uni::Type tc_pop_raw(std::vector<uni::Type>& stack) {
    auto type = stack.back();
    stack.pop_back();

    return type;
}

static uni::Type tc_pop_underflow(std::vector<uni::Type>& stack, size_t* bind_counter) {
    if(!stack.empty()) return tc_pop_raw(stack);
    return UNI_TYPE_VAR((*bind_counter)++);
}

static uni::Type tc_pop(uni::TcContext& ctx) {
    if(ctx.bind_counter) return tc_pop_underflow(ctx.stack, ctx.bind_counter);
    return tc_pop_raw(ctx.stack);
}

static void tc_cloneCtx(uni::TcContext& ctx, uni::TcContext& dst, size_t* bind_counter) {
    dst.stack = ctx.stack;
    dst.bind_counter = ctx.bind_counter? bind_counter : nullptr;
    dst.variables = ctx.variables;
    dst.is_global = ctx.is_global;
}
#define CLONE_CTX(dst, src)                             \
    uni::TcContext dst;                                 \
    size_t dst##_local_bind_counter_ = 0;             \
    tc_cloneCtx(src, dst, &dst##_local_bind_counter_)

static bool tc_apply_word(
    uni::TcContext& ctx,
    const std::vector<uni::Type>& inputs,
    const std::vector<uni::Type>& outputs,
    size_t line,
    std::string_view name
) {
    if(!ctx.bind_counter && ctx.stack.size() < inputs.size()) {
        std::cerr   << "[ERROR] (line " << line << ") '"
                    << name << "' needs " << inputs.size()
                    << " value(s) but stack only has " << ctx.stack.size() << '\n';
        return false;
    }

    std::unordered_map<size_t, uni::Type> bindings;
    size_t external_bindings = 0;
    for(size_t i = 0; i < inputs.size(); i++) {
        uni::Type actual = tc_pop(ctx);
        uni::Type expected = inputs[inputs.size() - 1 - i];

        if(expected.kind == uni::TypeKind::UNI_KIND_VAR) {
            size_t bid = expected.bind_id;
            if(bindings.find(bid) == bindings.end()) {
                bindings.insert({bid, actual});
            } else {
                if(!tc_kinds_compatible(actual.kind, bindings.at(bid).kind)) {
                    std::cerr   << "[ERROR] (line " << line
                                << ") '" << name << "' type mismatch: "
                                << "type variable '" << 'A' + bid << "' was bound to "
                                << type_name(bindings.at(bid).kind) << "but got "
                                << type_name(actual.kind) << '\n';
                    return false;
                }
            }
        } else if(actual.kind == uni::TypeKind::UNI_KIND_VAR) {
            size_t bid = actual.bind_id;
            if(
                bindings.find(bid) == bindings.end() &&
                expected.kind != uni::TypeKind::UNI_KIND_VAR
            ) {
                bindings.insert({bid, expected});
                ctx.bindings.insert({external_bindings++, expected});
            } else {
                if(!tc_kinds_compatible(expected.kind, bindings.at(bid).kind)) {
                    std::cerr   << "[ERROR] (line " << line
                                << ") '" << name << "' type mismatch: "
                                << "expected " << type_name(expected.kind)
                                << " but got " << type_name(bindings.at(bid).kind) << '\n';
                    return false;
                }
            }
        } else if(expected.kind == uni::TypeKind::UNI_KIND_NUM) {
            size_t bid = expected.bind_id;
            if(!tc_kinds_compatible(actual.kind, uni::TypeKind::UNI_KIND_NUM)) {
                std::cerr   << "[ERROR] (line " << line
                            << ") '" << name << "' type mismatch: "
                            << "expected number but got " << type_name(actual.kind) << '\n';
                return false;
            }

            if(bindings.find(bid) == bindings.end()) {
                bindings.insert({bid, actual});
            } else if(actual.kind == uni::TypeKind::UNI_KIND_FLOAT) {
                bindings.insert_or_assign(bid, actual);
            } else {
                if(!tc_kinds_compatible(actual.kind, expected.kind)) {
                    std::cerr   << "[ERROR] (line " << line
                                << ") '" << name << "' type mismatch: "
                                << "expected " << type_name(expected.kind)
                                << "but got " << type_name(actual.kind) << '\n';
                    return false;
                }
            }
        } else {
            if(!tc_kinds_compatible(actual.kind, expected.kind)) {
                std::cerr   << "[ERROR] (line " << line
                            << ") '" << name << "' type mismatch: "
                            << "expected " << type_name(expected.kind) << " but got "
                            << type_name(actual.kind) << '\n';
                return false;
            }
        }
    }

    for(size_t i = 0; i < outputs.size(); i++) {
        uni::Type out = outputs[i];
        if(
            out.kind == uni::TypeKind::UNI_KIND_VAR ||
            out.kind == uni::TypeKind::UNI_KIND_NUM
        ) {
            size_t bid = out.bind_id;
            if(bindings.find(bid) == bindings.end()) {
                std::cerr   << "[ERROR] (line " << line
                            << ") '" << name << "' output type variable '"
                            << 'A' + bid << "' is unbound (word definition error)\n";
                return false;
            }

            out = bindings.at(bid);
        }

        tc_push(ctx.stack, out);
    }

    return true;
}

static bool resolveTypeName(std::string_view name, uni::Type* out) {
    if(name == "int") {
        *out = UNI_TYPE_INT;
        return true;
    } else if(name == "float") {
        *out = UNI_TYPE_FLOAT;
        return true;
    } else if(name == "string") {
        *out = UNI_TYPE_STRING;
        return true;
    }

    return false;
}

static bool tc_op(uni::TcContext& ctx, const uni::Op* raw_op) {
    switch(raw_op->type) {
        case uni::OpType::UNI_OP_PUSH_INT: tc_push(ctx.stack, UNI_TYPE_INT); return true;
        case uni::OpType::UNI_OP_PUSH_FLOAT: tc_push(ctx.stack, UNI_TYPE_FLOAT); return true;
        case uni::OpType::UNI_OP_PUSH_STR: tc_push(ctx.stack, UNI_TYPE_STRING); return true;

        case uni::OpType::UNI_OP_WORD: {
            auto op = dynamic_cast<const uni::OpWord*>(raw_op);

            auto var = std::find_if(
                ctx.variables.begin(), ctx.variables.end(),
                [op](const uni::Variable& v) {
                    return v.name == op->name;
                }
            );
            if(var != ctx.variables.end()) {
                tc_push(ctx.stack, var->type);
                return true;
            }

            uni::Word* word = uni::lookupWord(op->name);
            if(!word) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") Unknown word '" << op->name << "'\n";
                return false;
            }

            return tc_apply_word(
                ctx,
                word->inputs,
                word->outputs,
                raw_op->line,
                word->name
            );
        }

        case uni::OpType::UNI_OP_BLOCK: {
            auto op = dynamic_cast<const uni::OpBlock*>(raw_op);
            return tc_block(ctx, op);
        }

        case uni::OpType::UNI_OP_IF: {
            if(!ctx.bind_counter && ctx.stack.size() == 0) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") 'if' needs a condition but stack is empty\n";
                return false;
            }

            uni::Type cond = tc_pop(ctx);
            if(cond.kind != uni::TypeKind::UNI_KIND_INT) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") 'if' condition must be int, got "
                            << type_name(cond.kind) << '\n';
                return false;
            }

            auto op = dynamic_cast<const uni::OpIf*>(raw_op);

            CLONE_CTX(then_ctx, ctx);
            if(!tc_block(then_ctx, op->then_body.get())) return false;

            if(!op->else_body) {
                if(!ctx.bind_counter && then_ctx.stack.size() != ctx.stack.size()) {
                    std::cerr   << "[ERROR] (line " << raw_op->line
                                << ") 'if' without 'else' must be stack-neutral: "
                                << "started with " << ctx.stack.size() << " items, "
                                << "ended with " << then_ctx.stack.size() << '\n';
                    return false;
                }

                return true;
            }

            CLONE_CTX(else_ctx, ctx);
            if(!tc_block(else_ctx, op->else_body.get())) return false;

            if(then_ctx.stack.size() != else_ctx.stack.size()) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") 'if/else' branches produce different stack depths: "
                            << "then=" << then_ctx.stack.size()
                            << ", else=" << else_ctx.stack.size() << '\n';
                return false;
            }

            for(size_t i = 0; i < then_ctx.stack.size(); i++) {
                if(then_ctx.stack[i].kind != else_ctx.stack[i].kind) {
                    std::cerr   << "[ERROR] (line " << raw_op->line
                                << ") 'if/else' branch produce different types at stack position " << i
                                << ": then=" << type_name(then_ctx.stack[i].kind)
                                <<", else=" << type_name(else_ctx.stack[i].kind) << '\n';
                    return false;
                }
            }

            ctx.stack = std::move(then_ctx.stack);
            return true;
        }

        case uni::OpType::UNI_OP_WHILE: {
            auto op = dynamic_cast<const uni::OpWhile*>(raw_op);

            CLONE_CTX(cond_ctx, ctx);
            if(!tc_block(cond_ctx, op->cond.get())) return false;

            if(!ctx.bind_counter && (
                cond_ctx.stack.size() != ctx.stack.size()+1 ||
                cond_ctx.stack[cond_ctx.stack.size()-1].kind != uni::TypeKind::UNI_KIND_INT
            )) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") 'while' condition block must leave exactly one int on the stack\n";
                return false;
            }

            if(
                ctx.bind_counter &&
                cond_ctx.stack.size() > 0 &&
                cond_ctx.stack[cond_ctx.stack.size()-1].kind != uni::TypeKind::UNI_KIND_INT
            ) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") 'while' condition block must leave exactly one int on the stack\n";
                return false;
            }

            CLONE_CTX(body_ctx, ctx);
            if(!tc_block(body_ctx, op->loop.get())) return false;

            if(!ctx.bind_counter && body_ctx.stack.size() != ctx.stack.size()) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") 'while' body must be stack-neutral\n";
                return false;
            }

            return true;
        }

        case uni::OpType::UNI_OP_DEF: {
            auto op = dynamic_cast<const uni::OpDef*>(raw_op);

            if(ctx.bind_counter) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") Nested word definitions are not allowed\n";
                return false;
            }

            std::string_view name = op->name;
            if(uni::lookupWord(name)) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") Duplicate word '" << op->name << "'\n";
                return false;
            }

            size_t bind_counter = 0;
            uni::TcContext inf_ctx = uni::TcContext{
                .bind_counter = &bind_counter,
                .variables = ctx.variables
            };
            if(!tc_block(inf_ctx, op->body.get())) return false;

            uni::TcContext check_ctx = uni::TcContext{
                .bind_counter = nullptr,
                .variables = ctx.variables
            };

            for(size_t i = 0; i < bind_counter; i++)
                tc_push(check_ctx.stack, UNI_TYPE_VAR(i));
            if(!tc_block(check_ctx, op->body.get())) return false;

            std::vector<uni::Type> inputs;
            for(size_t i = 0; i < bind_counter; i++) {
                inputs.push_back(
                    (check_ctx.bindings.find(i) != check_ctx.bindings.end())?
                        check_ctx.bindings.at(i) : UNI_TYPE_VAR(i)
                );
            }

            std::vector<uni::Type> outputs;
            for(size_t i = 0; i < check_ctx.stack.size(); i++) {
                uni::Type t = check_ctx.stack[i];
                if(
                    (t.kind == uni::TypeKind::UNI_KIND_VAR || t.kind == uni::TypeKind::UNI_KIND_NUM) &&
                    check_ctx.bindings.find(i) != check_ctx.bindings.end()
                ) {
                    t = check_ctx.bindings.at(i);
                }

                outputs.push_back(t);
            }

            uni::Word word{
                name,
                inputs, outputs,
                nullptr,
                op->body.get()
            };
            uni::registerWord(word);

            return true;
        }

        case uni::OpType::UNI_OP_LET: {
            auto op = dynamic_cast<const uni::OpLet*>(raw_op);

            if(ctx.is_global) {
                uni::Type bind_type;
                if(!resolveTypeName(op->type_name, &bind_type)) {
                    std::cerr   << "[ERROR] (line " << raw_op->line
                                << ") Unknown type name for variable: '"
                                << op->type_name << "'\n";
                    return false;
                }

                ctx.variables.push_back({
                    .name = op->name,
                    .type = bind_type,
                    .is_mut = op->is_mut,
                    .is_global = ctx.is_global
                });

                return true;
            } else {
                // TODO: For now, all bindings will be global until functions are implemented
                return false;
            }
        }

        case uni::OpType::UNI_OP_STORE: {
            auto op = dynamic_cast<const uni::OpStore*>(raw_op);

            auto var = std::find_if(
                ctx.variables.begin(), ctx.variables.end(),
                [op](const uni::Variable& var) {
                    return var.name == op->name;
                }
            );
            if(var == ctx.variables.end()) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") Unknown variable '" << op->name << "'\n";
                return false;
            }

            if(!var->is_mut) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") Cannot store into immutable variable '" << op->name << "'\n";
                return false;
            }

            if(ctx.stack.size() == 0) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") 'store' required a value on the stack\n";
                return false;
            }

            uni::Type actual = tc_pop(ctx);
            if(actual.kind != var->type.kind) {
                std::cerr   << "[ERROR] (line " << raw_op->line
                            << ") 'store' type mismatch: "
                            << "expected " << type_name(var->type.kind)
                            << " but got " << type_name(actual.kind) << '\n';
                return false;
            }

            return true;
        }
    }
}

static bool tc_block(uni::TcContext& ctx, const uni::OpBlock* block) {
    for(auto& op : block->items) {
        if(!tc_op(ctx, op.get())) return false;
    }

    return true;
}

