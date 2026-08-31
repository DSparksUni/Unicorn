#pragma once

#include <memory>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include "parser.hpp"

namespace uni {
    struct EmitVariable {
        std::string_view name;
        llvm::Value* ptr;
    };

    struct Emitter {
        llvm::LLVMContext ctx;
        std::unique_ptr<llvm::Module> module;
        llvm::IRBuilder<> builder{ctx};
        llvm::Function* func;

        std::vector<llvm::GlobalVariable*> str_globals;
        std::vector<llvm::Value*> stack;

        std::vector<EmitVariable> variables;

        llvm::FunctionCallee printf_fn;
        llvm::GlobalVariable* fmt_int;
        llvm::GlobalVariable* fmt_flt;
        llvm::GlobalVariable* fmt_str;

        Emitter();
        void push(llvm::Value* val);
        llvm::Value* pop();
    };

    bool writeProgram(Emitter& emitter, const std::string& out_path);

    void emitOp(Emitter& emitter, Op* op);
    void emitBlock(Emitter& emitter, OpBlock* block);
    void emitProgram(Emitter& emitter, OpBlock* program);
}
