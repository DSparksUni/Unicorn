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

#include <vector>
#include <string>
#include <optional>
#include <memory>

#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Passes/OptimizationLevel.h>

namespace uni {
    struct TempFile {
        const std::string& path;
        ~TempFile();
    };

    static inline std::optional<llvm::OptimizationLevel> getOptLevelFromString(
        const std::string& str
    ) {
        if     (str == "0") return llvm::OptimizationLevel::O0;
        else if(str == "1") return llvm::OptimizationLevel::O1;
        else if(str == "2") return llvm::OptimizationLevel::O2;
        else if(str == "3") return llvm::OptimizationLevel::O3;
        else if(str == "s") return llvm::OptimizationLevel::Os;
        else if(str == "z") return llvm::OptimizationLevel::Oz;

        return std::nullopt;
    }

    std::unique_ptr<llvm::TargetMachine> getSystemInfo(
        llvm::Module* module,
        llvm::OptimizationLevel opt_level
    );

    bool optimize(
        llvm::Module* module,
        llvm::TargetMachine* machine,
        llvm::OptimizationLevel opt_level
    );

    bool emitObject(
        llvm::Module* module,
        llvm::TargetMachine* machine,
        const std::string& out_obj_path
    );

    struct LibInfo {
        std::vector<std::string> lib_paths;
        std::vector<std::string> libs;
        std::vector<std::string> link_args;
        std::vector<std::string> before_objs;
        std::vector<std::string> after_objs;

    };
    std::optional<LibInfo> getLibInfo();

    bool link(
        const std::string& obj_path,
        const std::string& out_exe_path,
        const LibInfo& info
    );
}

