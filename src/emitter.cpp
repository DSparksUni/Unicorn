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

#include "emitter.hpp"

#include <iostream>
#include <system_error>

#include <llvm/Support/raw_ostream.h>

#include "word.hpp"
#include "llvm-c/Types.h"

static void collect_strings(uni::Emitter& emitter, uni::Op* raw_op);

namespace uni {
    Emitter::Emitter():
        module{std::make_unique<llvm::Module>("uni", ctx)}
    {
        llvm::FunctionType* main_type = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(ctx), false
        );
        func = llvm::Function::Create(
            main_type, llvm::Function::ExternalLinkage, "main", module.get()
        );
        llvm::BasicBlock* entry = llvm::BasicBlock::Create(ctx, "entry", func);
        builder.SetInsertPoint(entry);

        llvm::FunctionType* printf_type = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(ctx), {llvm::PointerType::get(ctx, 0)}, true
        );
        printf_fn = module->getOrInsertFunction("printf", printf_type);

        fmt_int = builder.CreateGlobalString("%lld", "fmt_int");
        fmt_flt = builder.CreateGlobalString("%f", "fmt_flt");
        fmt_str = builder.CreateGlobalString("%s", "fmt_str");
    }
    void Emitter::push(llvm::Value* val) {
        stack.push_back(val);
    }
    llvm::Value* Emitter::pop() {
        if(stack.empty()) return nullptr;

        llvm::Value* val = stack.back();
        stack.pop_back();
        return val;
    }

    bool writeProgram(Emitter& emitter, const std::string& out_path) {
        std::error_code ec;
        llvm::raw_fd_ostream out(out_path, ec);
        if(ec) {
            std::cerr   << "[ERROR] Failed to write IR: " << ec.message() << '\n';
            return false;
        }

        emitter.module->print(out, nullptr);
        return true;
    }

    void emitOp(Emitter& emitter, Op* raw_op) {
        switch(raw_op->type) {
            case OpType::UNI_OP_PUSH_INT: {
                auto op = dynamic_cast<const OpPushInt*>(raw_op);

                llvm::Value* val = llvm::ConstantInt::get(
                    llvm::Type::getInt64Ty(emitter.ctx), op->value, true
                );
                emitter.push(val);
            } break;

            case OpType::UNI_OP_PUSH_FLOAT: {
                auto op = dynamic_cast<const OpPushFloat*>(raw_op);

                llvm::Value* val = llvm::ConstantFP::get(
                    llvm::Type::getDoubleTy(emitter.ctx), op->value
                );
                emitter.push(val);
            } break;

            case OpType::UNI_OP_PUSH_STR: {
                auto op = dynamic_cast<const OpPushStr*>(raw_op);

                llvm::Value* val = emitter.str_globals[op->global_idx];
                emitter.push(val);
            } break;

            case OpType::UNI_OP_WORD: {
                auto op = dynamic_cast<OpWord*>(raw_op);

                auto var_it = std::find_if(
                    emitter.variables.begin(), emitter.variables.end(),
                    [op](const EmitVariable& var) {
                        return var.name == op->name;
                    }
                );
                if(var_it != emitter.variables.end()) {
                    llvm::GlobalVariable* global = llvm::cast<llvm::GlobalVariable>(var_it->ptr);
                    llvm::Value* val = emitter.builder.CreateLoad(
                        global->getValueType(),
                        global,
                        "load_variable"
                    );
                    emitter.push(val);
                    break;
                }

                Word* word = lookupWord(op->name);
                if(word && word->body) {
                    auto blk = dynamic_cast<OpBlock*>(word->body);
                    emitBlock(emitter, blk);
                } else if(word) {
                    word->emit(emitter);
                } else {
                    std::cerr   << "[ERROR] (line " << op->line
                                << ") Unknown word '" << op->name
                                << "' got past typechecking\n";
                }
            } break;

            case OpType::UNI_OP_BLOCK: break;

            case OpType::UNI_OP_IF: {
                auto op = dynamic_cast<const OpIf*>(raw_op);

                llvm::Value* cond = emitter.pop();
                llvm::Value* cond_bool = emitter.builder.CreateICmpNE(
                    cond,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.ctx), 0, false),
                    "cond"
                );

                llvm::BasicBlock* then_block = llvm::BasicBlock::Create(
                    emitter.ctx, "then", emitter.func
                );
                llvm::BasicBlock* else_block = (op->else_body)?
                    llvm::BasicBlock::Create(emitter.ctx, "else", emitter.func)
                    : nullptr;
                llvm::BasicBlock* end_block = llvm::BasicBlock::Create(
                    emitter.ctx, "end", emitter.func
                );

                if(op->else_body) {
                    emitter.builder.CreateCondBr(cond_bool, then_block, else_block);
                } else {
                    emitter.builder.CreateCondBr(cond_bool, then_block, end_block);
                }

                size_t stack_depth_before = emitter.stack.size();
                std::vector<llvm::Value*> pre_vals;
                if(stack_depth_before > 0) pre_vals = emitter.stack;
                llvm::BasicBlock* pre_block = emitter.builder.GetInsertBlock();

                emitter.builder.SetInsertPoint(then_block);
                emitBlock(emitter, op->then_body.get());
                emitter.builder.CreateBr(end_block);

                size_t then_stack_count = emitter.stack.size();
                std::vector<llvm::Value*> then_vals;
                if(then_stack_count > 0) then_vals = emitter.stack;

                if(op->else_body) {
                    emitter.stack = pre_vals;

                    emitter.builder.SetInsertPoint(else_block);
                    emitBlock(emitter, op->else_body.get());
                    emitter.builder.CreateBr(end_block);

                    size_t else_stack_count = emitter.stack.size();
                    std::vector<llvm::Value*> else_vals;
                    if(else_stack_count > 0) else_vals = emitter.stack;

                    emitter.builder.SetInsertPoint(end_block);

                    emitter.stack.clear();
                    if(then_stack_count && else_stack_count) {
                        for(size_t i = 0; i < then_stack_count; i++) {
                            llvm::PHINode* phi = emitter.builder.CreatePHI(
                                then_vals[i]->getType(), 2, "phi"
                            );
                            phi->addIncoming(then_vals[i], then_block);
                            phi->addIncoming(else_vals[i], else_block);
                            emitter.push(phi);
                        }
                    }
                } else {
                    emitter.builder.SetInsertPoint(end_block);

                    emitter.stack.clear();
                    for(size_t i = 0; i < stack_depth_before; i++) {
                        llvm::PHINode* phi = emitter.builder.CreatePHI(
                            pre_vals[i]->getType(), 2, "phi"
                        );
                        phi->addIncoming(then_vals[i], then_block);
                        phi->addIncoming(pre_vals[i], pre_block);
                        emitter.push(phi);
                    }
                }
            } break;

            case OpType::UNI_OP_WHILE: {
                auto op = dynamic_cast<const OpWhile*>(raw_op);

                llvm::BasicBlock* pre_block = emitter.builder.GetInsertBlock();
                llvm::BasicBlock* cond_block = llvm::BasicBlock::Create(
                    emitter.ctx, "while_cond", emitter.func
                );
                llvm::BasicBlock* body_block = llvm::BasicBlock::Create(
                    emitter.ctx, "while_body", emitter.func
                );
                llvm::BasicBlock* end_block = llvm::BasicBlock::Create(
                    emitter.ctx, "while_end", emitter.func
                );

                emitter.builder.CreateBr(cond_block);
                emitter.builder.SetInsertPoint(cond_block);

                size_t stack_base = emitter.stack.size();
                std::vector<llvm::PHINode*> phis;
                for(size_t i = 0; i < stack_base; i++) {
                    llvm::PHINode* phi = emitter.builder.CreatePHI(
                        emitter.stack[i]->getType(), 2, "loop_var"
                    );
                    phi->addIncoming(emitter.stack[i], pre_block);
                    phis.push_back(phi);
                    emitter.stack[i] = phi;
                }

                emitBlock(emitter, op->cond.get());
                llvm::Value* cond_val = emitter.pop();
                llvm::Value* cond_bool = emitter.builder.CreateICmpNE(
                    cond_val,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(emitter.ctx), 0, false),
                    "while_cond_bool"
                );
                emitter.builder.CreateCondBr(cond_bool, body_block, end_block);

                emitter.builder.SetInsertPoint(body_block);
                emitBlock(emitter, op->loop.get());

                llvm::BasicBlock* body_end = emitter.builder.GetInsertBlock();
                for(size_t i = 0; i < stack_base; i++)
                    phis[i]->addIncoming(emitter.stack[i], body_end);

                emitter.builder.CreateBr(cond_block);
                emitter.builder.SetInsertPoint(end_block);
            } break;

            case OpType::UNI_OP_DEF: break;

            case OpType::UNI_OP_LET: {
                auto op = dynamic_cast<const OpLet*>(raw_op);

                llvm::Type* var_type;
                if(op->type_name == "int") {
                    var_type = llvm::Type::getInt64Ty(emitter.ctx);
                } else if(op->type_name == "float") {
                    var_type = llvm::Type::getDoubleTy(emitter.ctx);
                } else if(op->type_name == "string") {
                    var_type = llvm::PointerType::get(emitter.ctx, 0);
                } else {
                    std::cerr   << "[ERROR] (line " << raw_op->line
                                << ") Unknown type '" << op->type_name
                                << "' for variable definition\n";
                    break;
                }

                llvm::Constant* var_init = llvm::Constant::getNullValue(var_type);
                llvm::GlobalVariable* variable = new llvm::GlobalVariable(
                    *emitter.module,
                    var_type,
                    false,
                    llvm::GlobalValue::ExternalLinkage,
                    var_init,
                    "var"
                );

                emitter.variables.push_back({
                    .name = op->name,
                    .ptr = variable
                });
            } break;

            case uni::OpType::UNI_OP_STORE: {
                auto op = dynamic_cast<const uni::OpStore*>(raw_op);

                auto var_it = std::find_if(
                    emitter.variables.begin(), emitter.variables.end(),
                    [op](const uni::EmitVariable& var) {
                        return var.name == op->name;
                    }
                );
                if(var_it != emitter.variables.end()) {
                    llvm::Value* val = emitter.pop();
                    emitter.builder.CreateStore(
                        val,
                        var_it->ptr
                    );
                }
            } break;
        }
    }

    void emitBlock(Emitter& emitter, OpBlock* block) {
        for(auto& op : block->items) emitOp(emitter, op.get());
    }

    void emitProgram(Emitter& emitter, OpBlock* program) {
        collect_strings(emitter, program);

        emitBlock(emitter, program);

        emitter.builder.CreateRet(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(emitter.ctx), 0, true)
        );
    }
}

static void collect_strings(uni::Emitter& emitter, uni::Op* raw_op) {
    switch(raw_op->type) {
        case uni::OpType::UNI_OP_PUSH_STR: {
            auto op = dynamic_cast<uni::OpPushStr*>(raw_op);

            llvm::GlobalVariable* str = emitter.builder.CreateGlobalString(op->value, "str");
            emitter.str_globals.push_back(str);
            op->global_idx = emitter.str_globals.size()-1;
        } break;

        case uni::OpType::UNI_OP_BLOCK: {
            auto op = dynamic_cast<const uni::OpBlock*>(raw_op);
            for(auto& op : op->items) collect_strings(emitter, op.get());
        } break;

        case uni::OpType::UNI_OP_IF: {
            auto op = dynamic_cast<const uni::OpIf*>(raw_op);

            collect_strings(emitter, op->then_body.get());
            if(op->else_body) collect_strings(emitter, op->else_body.get());
        } break;

        case uni::OpType::UNI_OP_WHILE: {
            auto op = dynamic_cast<const uni::OpWhile*>(raw_op);

            collect_strings(emitter, op->cond.get());
            collect_strings(emitter, op->loop.get());
        } break;

        case uni::OpType::UNI_OP_DEF: {
            auto op = dynamic_cast<const uni::OpDef*>(raw_op);
            collect_strings(emitter, op->body.get());
        } break;

        default: break;
    }
}
