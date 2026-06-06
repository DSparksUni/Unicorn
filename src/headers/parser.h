#ifndef UNI_PARSER_H_INCLUDED_
#define UNI_PARSER_H_INCLUDED_

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "lexer.h"

typedef struct uniParser_t {
    uniToken* tokens;
    size_t num_tokens;
    size_t cursor;
} uniParser;

uniParser* uni_createParser(uniToken* tokens, size_t token_len);
void uni_destroyParser(uniParser* parser);

uniToken uni_peekParser(uniParser* parser);
uniToken uni_advanceParser(uniParser* parser);
bool uni_expectParser(uniParser* parser, uniTokenType type);

typedef enum uniOpType_t {
    UNI_OP_PUSH_INT,
    UNI_OP_PUSH_FLOAT,
    UNI_OP_PUSH_STR,
    UNI_OP_WORD,
    UNI_OP_BLOCK,
    UNI_OP_IF,
    UNI_OP_WHILE,
    UNI_OP_DEF,
} uniOpType;

typedef struct uniOp_t {
    uniOpType type;
    size_t line;
    union {
        int64_t ival;
        double fval;
        struct {
            const char* start;
            size_t len;
            size_t global_idx;
        } sval;
        struct {
            struct uniOp_t** items;
            size_t num_items;
        } bval;
        struct {
            struct uniOp_t* then_body;
            struct uniOp_t* else_body;
        } cval;
        struct {
            struct uniOp_t* cond_body;
            struct uniOp_t* loop_body;
        } wval;
        struct {
            const char* name;
            size_t name_len;
            struct uniOp_t* body;
        } dval;
    };
} uniOp;

uniOp* uni_parseBlock(uniParser* parser);
uniOp* uni_parseOne(uniParser* parser);
uniOp* uni_parseProgram(uniParser* parser);

void uni_destroyOp(uniOp* op);
void uni_printOp(uniOp* op, size_t indent);

#endif
