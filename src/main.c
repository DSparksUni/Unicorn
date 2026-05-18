#include <stdio.h>
#include <stdlib.h>

#include "headers/util.h"
#include "headers/lexer.h"
#include "headers/parser.h"

int main(int argc, char** argv) {
    if(argc < 2) {
        fprintf(stderr, "[ERROR] No input file supplied...\n");
        return -1;
    }
    char* input_file = argv[1];

    size_t content_size;
    char* content = uni_readFile(input_file, &content_size);
    if(!content) {
        fprintf(stderr, "[ERROR] Failed to read file '%s'\n", input_file);
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

    uni_printOp(program, 0);

    uni_destroyOp(program);
    uni_destroyParser(parser);
    free(content);
    return 0;
}
