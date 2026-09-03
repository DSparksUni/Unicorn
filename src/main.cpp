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

#include <iostream>
#include <optional>

#include <cxxopts.hpp>

#include "util.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "typecheck.hpp"
#include "emitter.hpp"

struct Input {
    std::string in_file;
    std::string out_file;
    bool print_tokens, print_ops;
};
std::optional<Input> parse_args(int argc, char** argv);

int main(int argc, char** argv) {
    auto parsed_input = parse_args(argc, argv);
    if(!parsed_input) return -1;
    Input input = parsed_input.value();

    auto read_result = uni::readFile(input.in_file);
    if(!read_result) {
        std::cerr << "[ERROR] Failed to read file '" << input.in_file << "'\n";
        return -1;
    }
    std::string input_content = read_result.value();

    auto lex_result = uni::lex(input_content);
    if(!lex_result) return -1;
    std::vector<uni::Token> tokens = lex_result.value();
    if(input.print_tokens) {
        for(const auto& tok : tokens) std::cout << tok.toString();
    }

    uni::Parser parser(tokens);
    std::unique_ptr<uni::OpBlock> program = parser.parseProgram();
    if(!program) return -1;
    if(input.print_ops) std::cout << uni::opToString(program.get()) << '\n';

    if(!uni::typecheck(program.get())) return -1;

    uni::Emitter emitter;
    uni::emitProgram(emitter, program.get());
    uni::writeProgram(emitter, "out.ll");

    std::string cmd = "clang -O1 out.ll -o " + input.out_file;
    std::system(cmd.c_str());
    std::remove("out.ll");

    return 0;
}

std::optional<Input> parse_args(int argc, char** argv) {
    cxxopts::Options options("uni", "Stack-based language");
    options.add_options()
        ("in-file", "Input file", cxxopts::value<std::string>())
        ("o,out-file", "Output file", cxxopts::value<std::string>()->default_value("out.exe"))
        ("h,help", "Print help message")
        ("print-tokens", "Print generated tokens")
        ("print-ops", "Print generated ast")
    ;
    options.parse_positional({"in-file"});

    cxxopts::ParseResult args_result;
    try {
        args_result = options.parse(argc, argv);
    } catch(cxxopts::exceptions::exception e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return std::nullopt;
    }

    if(args_result.count("help")) {
        std::cout << options.help() << '\n';
        exit(0);
    }

    if(!args_result.count("in-file")) {
        std::cerr << "[ERROR] No input file supplied...\n";
        return std::nullopt;
    }

    return Input{
        .in_file{args_result["in-file"].as<std::string>()},
        .out_file{args_result["out-file"].as<std::string>()},
        .print_tokens = args_result.contains("print-tokens"),
        .print_ops = args_result.contains("print-ops"),
    };
}
