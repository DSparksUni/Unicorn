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

#include <llvm/TargetParser/Triple.h>
#include <llvm/IR/Module.h>

namespace uni {
    bool emitObject(llvm::Module* module, const std::string& out_obj_path);

    static inline std::string getArchString(llvm::Triple::ArchType arch) {
        switch(arch) {
            case llvm::Triple::ArchType::x86:       return "x86";
            case llvm::Triple::ArchType::x86_64:    return "x64";
            case llvm::Triple::ArchType::aarch64:   return "arm64";
            case llvm::Triple::ArchType::arm:       return "arm";

            default: return "";
        }
    }

    struct LibInfo {
        std::vector<std::string> lib_paths;
        std::vector<std::string> libs;
    };
    std::optional<LibInfo> getLibInfo();

    bool link(
        const std::string& obj_path,
        const std::string& out_exe_path,
        const LibInfo& info
    );
}

