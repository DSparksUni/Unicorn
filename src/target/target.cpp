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
#include <filesystem>

#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <lld/Common/Driver.h>

#ifdef _WIN32
LLD_HAS_DRIVER(coff)
namespace lld_platform = lld::coff;
static std::string LIB_PATH_PREFIX = "/libpath:";
static std::string LINK_PROGRAM = "lld-link";

#elif defined(__APPLE__)
LLD_HAS_DRIVER(macho)
namespace lld_platform = lld::macho;
static std::string LIB_PATH_PREFIX = "-L";
static std::string LINK_PROGRAM = "ld64.lld";

#elif defined(__linux__)
LLD_HAS_DRIVER(elf)
namespace lld_platform = lld::elf;
static std::string LIB_PATH_PREFIX = "-L";
static std::string LINK_PROGRAM = "ld.lld";

#endif

static llvm::CodeGenOptLevel getOptLevel(llvm::OptimizationLevel opt);

namespace uni {
    TempFile::~TempFile() {
        std::filesystem::remove(path);
    }

    std::unique_ptr<llvm::TargetMachine> getSystemInfo(
        llvm::Module* module,
        llvm::OptimizationLevel opt_level
    ) {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        std::string triple = llvm::sys::getDefaultTargetTriple();

        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);
        if(!target) {
            std::cerr   << "[ERROR] Failed to lookup target: "
                        << error << '\n';
            return nullptr;
        }

        llvm::CodeGenOptLevel gen_level = getOptLevel(opt_level);
        llvm::TargetOptions opts;
        std::unique_ptr<llvm::TargetMachine> machine(target->createTargetMachine(
            llvm::Triple{triple},
            "generic",
            "",
            opts,
            llvm::Reloc::PIC_,
            std::nullopt,
            gen_level
        ));
        if(!machine) {
            std::cerr   << "[ERROR] Failed to initialize target machine\n";
            return nullptr;
        }

        module->setTargetTriple(llvm::Triple{triple});
        module->setDataLayout(machine->createDataLayout());

        return machine;
    }

    bool optimize(
        llvm::Module* module,
        llvm::TargetMachine* machine,
        llvm::OptimizationLevel opt_level
    ) {
        llvm::PassBuilder builder(machine);

        llvm::LoopAnalysisManager loop_man;
        llvm::FunctionAnalysisManager func_man;
        llvm::CGSCCAnalysisManager cgscc_man;
        llvm::ModuleAnalysisManager mod_man;

        builder.registerLoopAnalyses(loop_man);
        builder.registerFunctionAnalyses(func_man);
        builder.registerCGSCCAnalyses(cgscc_man);
        builder.registerModuleAnalyses(mod_man);
        builder.crossRegisterProxies(loop_man, func_man, cgscc_man, mod_man);

        llvm::ModulePassManager pass_manager;
        if(opt_level.getSpeedupLevel() == 0) {
            pass_manager = builder.buildO0DefaultPipeline(opt_level);
        } else {
            pass_manager = builder.buildPerModuleDefaultPipeline(opt_level);
        }

        pass_manager.run(*module, mod_man);
        return !llvm::verifyModule(*module, &llvm::errs());
    }

    bool emitObject(
        llvm::Module* module,
        llvm::TargetMachine* machine,
        const std::string& out_obj_path
    ) {
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
        link_args.push_back(LINK_PROGRAM);

        for(auto& obj : info.before_objs) link_args.push_back(obj);
        link_args.push_back(obj_path);
        for(auto& obj : info.after_objs) link_args.push_back(obj);

        #ifdef _WIN32
            link_args.push_back("/out:" + out_exe_path);
        #else
            link_args.push_back("-o");
            link_args.push_back(out_exe_path);
        #endif

        for(auto& arg : info.link_args) link_args.push_back(arg);
        for(auto& lp : info.lib_paths) link_args.push_back(LIB_PATH_PREFIX + lp);
        for(auto& l : info.libs) link_args.push_back(l);

        std::vector<const char*> argv;
        for(auto& arg : link_args) argv.push_back(arg.c_str());

        if(!lld_platform::link(
            argv, llvm::outs(), llvm::errs(), false, false
        )) {
            // LLD logs its own errors, no diagnostic needed
            return false;
        }

        return true;
    }
}

static llvm::CodeGenOptLevel getOptLevel(llvm::OptimizationLevel opt) {
    switch(opt.getSpeedupLevel()) {
        case 0: return llvm::CodeGenOptLevel::None;
        case 1: return llvm::CodeGenOptLevel::Less;
        case 3: return llvm::CodeGenOptLevel::Aggressive;

        default: return llvm::CodeGenOptLevel::Default;
    }
}

