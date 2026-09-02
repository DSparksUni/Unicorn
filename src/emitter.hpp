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
