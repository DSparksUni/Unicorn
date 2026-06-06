#ifndef UNI_LEXER_H_INCLUDED_
#define UNI_LEXER_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum uniTokenType_t {
    UNI_TOKEN_NULL,
    UNI_TOKEN_INT,
    UNI_TOKEN_FLOAT,
    UNI_TOKEN_STRING,
    UNI_TOKEN_WORD,
    UNI_TOKEN_LPAREN,
    UNI_TOKEN_RPAREN,
    UNI_TOKEN_LBRACE,
    UNI_TOKEN_RBRACE,
    UNI_TOKEN_EOF
} uniTokenType;

typedef struct uniToken_t {
    uniTokenType type;
    const char* start;
    size_t len;
    size_t line;
    union {
        int64_t ival;
        double fval;
    };
} uniToken;

size_t uni_lex(const char* src, uniToken* out, size_t max_tokens);

void uni_printToken(uniToken tok);

#endif
