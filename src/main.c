#include <stdio.h>
#include <stdlib.h>

#include "headers/args.h"
#include "headers/util.h"
#include "headers/lexer.h"
#include "headers/parser.h"
#include "headers/emitter.h"
#include "headers/typecheck.h"

int main(int argc, char** argv) {
    uniArgs args;
    if(!uni_parseArgs(argc, argv, &args)) {
        return -1;
    }

    size_t content_size;
    char* content = uni_readFile(args.input_file, &content_size);
    if(!content) {
        fprintf(stderr, "[ERROR] Failed to read file '%s'\n", args.input_file);
        return -1;
    }

    uniToken* tokens = malloc(content_size * sizeof(uniToken));
    if(!tokens) {
        fprintf(stderr, "[ERROR] Failed to allocate token buffer...\n");
        free(content);
        return -1;
    }
    size_t num_tokens = uni_lex(content, tokens, content_size);

    uniParser* parser = uni_createParser(tokens, num_tokens);
    if(!parser) {
        fprintf(stderr, "[ERROR] Failed to create parser...\n");
        free(tokens);
        free(content);
        return -1;
    }

    uniOp* program = uni_parseProgram(parser);
    if(!program) {
        fprintf(stderr, "[ERROR] Failed to parse program...\n");
        uni_destroyParser(parser);
        free(content);
        return -1;
    }

    if(!uni_typecheck(program)) {
        uni_destroyParser(parser);
        free(content);
        return -1;
    }

    uniEmitter* emitter = uni_createEmitter("out.ll");
    if(!emitter) {
        fprintf(stderr, "[ERROR] Failed to create emitter...\n");
        uni_destroyOp(program);
        uni_destroyParser(parser);
        free(content);
        return -1;
    }

    uni_emitProgram(emitter, program);
    uni_destroyEmitter(emitter);

    if(args.output_file) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "clang -O1 out.ll -o %s", args.output_file);
        system(cmd);
    } else {
        system("clang -O1 out.ll -o out.exe");
    }
    remove("out.ll");

    uni_destroyOp(program);
    uni_destroyParser(parser);
    free(content);
    return 0;
}
