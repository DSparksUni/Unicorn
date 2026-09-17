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

#include "target.hpp"

#include <iostream>
#include <tuple>

#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>
#include <lld/Common/Driver.h>

LLD_HAS_DRIVER(coff)

namespace uni {
    bool emitObject(llvm::Module* module, const std::string& out_obj_path) {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        std::string triple = llvm::sys::getDefaultTargetTriple();

        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);
        if(!target) {
            std::cerr   << "[ERROR] Failed to lookup target: "
                        << error << '\n';
            return false;
        }

        llvm::TargetOptions opts;
        llvm::TargetMachine* machine = target->createTargetMachine(
            llvm::Triple{triple}, "generic", "", opts, llvm::Reloc::PIC_
        );

        module->setTargetTriple(llvm::Triple{triple});
        module->setDataLayout(machine->createDataLayout());

        std::error_code ec;
        llvm::raw_fd_ostream dest(out_obj_path, ec, llvm::sys::fs::OF_None);
        if(ec) {
            std::cerr   << "[ERROR] Failed to open file '"
                        << out_obj_path << "': "
                        << ec.message() << '\n';
            return false;
        }

        llvm::legacy::PassManager pass;
        if(machine->addPassesToEmitFile(
            pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile
        )) {
            std::cerr   << "[ERROR] Target machine can't emit an object file of this type\n";
            return false;
        }

        pass.run(*module);
        dest.flush();
        return true;
    }

    bool link(
        const std::string& obj_path,
        const std::string& out_exe_path,
        const LibInfo& info
    ) {
        std::vector<std::string> link_args;
        link_args.push_back("lld-link");
        link_args.push_back(obj_path);
        link_args.push_back("/out:" + out_exe_path);
        link_args.push_back("/entry:mainCRTStartup");
        link_args.push_back("/subsystem:console");

        for(auto& lp : info.lib_paths) link_args.push_back("/libpath:" + lp);
        for(auto& l : info.libs) link_args.push_back(l);

        std::vector<const char*> argv;
        for(auto& arg : link_args) argv.push_back(arg.c_str());

        if(!lld::coff::link(
            argv, llvm::outs(), llvm::errs(), false, false
        )) {
            // LLD logs its own errors, no diagnostic needed
            return false;
        }

        return true;
    }
}


