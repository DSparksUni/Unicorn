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

#include "../target.hpp"

#include <cstdio>
#include <iostream>
#include <optional>
#include <filesystem>
#include <windows.h>

#include <llvm/TargetParser/Host.h>

static std::tuple<unsigned, unsigned, unsigned> getVersion3(const std::string& str);
static std::tuple<unsigned, unsigned, unsigned, unsigned> getVersion4(const std::string& str);

static bool getMSVCPath(uni::LibInfo& info, const std::string& arch);
static bool getSDKPath(uni::LibInfo& info, const std::string& arch);

namespace uni {
    std::optional<LibInfo> getLibInfo() {
        llvm::Triple target_triple{ llvm::sys::getDefaultTargetTriple() };
        std::string arch = getArchString(target_triple.getArch());

        LibInfo info;
        if(!getMSVCPath(info, arch)) return std::nullopt;
        if(!getSDKPath(info, arch)) return std::nullopt;

        info.libs = {
            "libcmt.lib",
            "libvcruntime.lib",
            "libucrt.lib",
            "kernel32.lib",
            "legacy_stdio_definitions.lib"
        };

        return info;
    }
}

static std::tuple<unsigned, unsigned, unsigned> getVersion3(const std::string& str) {
    unsigned v1, v2, v3;
    if(
        sscanf(str.c_str(), "%u.%u.%u", &v1, &v2, &v3) != 3
    ) {
        // Version string was not as expected, skip for now
        // (maybe do something else later)
        return {0, 0, 0};
    }

    return {v1, v2, v3};
}
static std::tuple<unsigned, unsigned, unsigned, unsigned> getVersion4(const std::string& str) {
    unsigned v1, v2, v3, v4;
    if(
        sscanf(str.c_str(), "%u.%u.%u.%u", &v1, &v2, &v3, &v4) != 4
    ) {
        // Same as above
        return {0, 0, 0, 0};
    }

    return {v1, v2, v3, v4};
}

static bool getMSVCPath(uni::LibInfo& info, const std::string& arch) {
    std::string cmd =   "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe\""
                            " -latest -products * -property installationPath";
    FILE* pipe = _popen(cmd.c_str(), "r");
    if(!pipe) {
        std::cerr   << "[ERROR] Failed to locate Visual Studio build tools\n";
        return false;
    }

    std::string vs_root;
    char buffer[512];
    while(fgets(buffer, sizeof(buffer), pipe)) vs_root += buffer;

    if(_pclose(pipe) != 0) {
        std::cerr  << "[ERROR] Failed to fetch Visual Studio location\n";
        return false;
    }

    vs_root.erase(vs_root.find_last_not_of(" \r\n\t") + 1);
    if(vs_root.empty()) {
        std::cerr   << "[ERROR] Failed to fetch Visual Studio location\n";
        return false;
    }

    std::tuple<unsigned, unsigned, unsigned> best_version{0, 0, 0};
    std::string best_name;

    std::string msvc_root = vs_root + "/VC/Tools/MSVC";
    if(!std::filesystem::exists(msvc_root)) {
        std::cerr   << "[ERROR] Detected Visual Studio root does not exist\n";
        return false;
    }

    for(
        auto& entry : std::filesystem::directory_iterator(msvc_root)
    ) {
        if(!entry.is_directory()) continue;
        auto name = entry.path().filename().string();
        auto v = getVersion3(name);
        if(v > best_version) {
            best_version = v;
            best_name = name;
        }
    }

    if(best_name.empty()) {
        std::cerr   << "[ERROR] Failed to find Visual Studio install\n";
        return false;
    }

    info.lib_paths.push_back(std::format("{}/{}/lib/{}", msvc_root, best_name, arch));
    return true;
}

static bool getSDKPath(uni::LibInfo& info, const std::string& arch) {
    const std::string key = "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots";    
    const std::string val = "KitsRoot10";

    static constexpr size_t MAX_PATH_SIZE = 512;
    char buffer[MAX_PATH_SIZE];

    DWORD num_bytes = MAX_PATH_SIZE;
    if(RegGetValueA(
        HKEY_LOCAL_MACHINE,
        key.c_str(),
        val.c_str(),
        RRF_RT_REG_SZ,
        nullptr,
        buffer,
        &num_bytes
    ) != 0) {
        std::cerr   << "[ERROR] Failed to load Windows SDK path\n";
        return false;
    }

    std::string sdk_root(buffer);
    if(sdk_root.empty()) {
        std::cerr   << "[ERROR] Failed to load Windows SDK path\n";
        return false;
    }

    std::tuple<unsigned, unsigned, unsigned, unsigned> best_version{0, 0, 0, 0};
    std::string best_name;

    std::string sdk_lib = sdk_root + "/Lib";
    if(!std::filesystem::exists(sdk_lib)) {
        std::cerr   << "[ERROR] Invalid Windows SDK path\n";
        return false;
    }

    for(
        auto& entry : std::filesystem::directory_iterator(sdk_lib)
    ) {
        if(!entry.is_directory()) continue;
        auto name = entry.path().filename().string();
        auto v = getVersion4(name);
        if(v > best_version) {
            best_version = v;
            best_name = name;
        }
    }

    if(best_name.empty()) {
        std::cerr   << "[ERROR] Failed to find Windows SDK\n";
        return false;
    }

    info.lib_paths.push_back(std::format(
        "{}/Lib/{}/um/{}",
        sdk_root, best_name, arch
    ));
    info.lib_paths.push_back(std::format(
        "{}/Lib/{}/ucrt/{}",
        sdk_root, best_name, arch
    ));

    return true;
}

