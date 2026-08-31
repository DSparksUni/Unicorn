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

    uni::Parser parser(tokens);
    std::unique_ptr<uni::OpBlock> program = parser.parseProgram();
    if(!program) return -1;

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
        .out_file{args_result["out-file"].as<std::string>()}
    };
}
