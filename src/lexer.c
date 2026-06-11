#include "headers/lexer.h"

static bool is_float(const char* p);

size_t uni_lex(const char* src, uniToken* out, size_t max_tokens) {
    const char* p = src;
    size_t line = 1;
    size_t n = 0;

    while(n < max_tokens) {
        // Skip whitespace
        while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
            if(*p == '\n') line++;
            p++;
        }

        // Check for EOF
        if(*p == '\0') {
            out[n++] = (uniToken){UNI_TOKEN_EOF, p, 0, line};
            break;
        }

        uniToken tok = {
            .start = p,
            .line = line
        };

        switch(*p) {
            case '(': {
                tok.type = UNI_TOKEN_LPAREN;
                tok.len = 1;
                p++;
            } break;
            case ')': {
                tok.type = UNI_TOKEN_RPAREN;
                tok.len = 1;
                p++;
            } break;
            case '{': {
                tok.type = UNI_TOKEN_LBRACE;
                tok.len = 1;
                p++;
            } break;
            case '}': {
                tok.type = UNI_TOKEN_RBRACE;
                tok.len = 1;
                p++;
            } break;

            case ':': {
                tok.type = UNI_TOKEN_COLON;
                tok.len = 1;
                p++;
            } break;

            case '\"': {
                const char* start = p++;
                while(*p && *p != '\"') {
                    if(*p == '\\') p++;     // Skip escape sequences
                    p++;
                }
                if(*p == '\"') p++;

                tok.type = UNI_TOKEN_STRING;
                tok.start = start;
                tok.len = (size_t)(p - start);
            } break;

            // Numbers
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                char* end;
                if(is_float(p)) {
                    tok.fval = strtod(p, &end);
                    tok.type = UNI_TOKEN_FLOAT;
                } else {
                    tok.ival = strtoll(p, &end, 0);
                    tok.type = UNI_TOKEN_INT;
                }
                tok.len = (size_t)(end - p);
                p = end;
            } break;

            default: {
                if(*p == '-' && p[1] == '>') {
                    tok.type = UNI_TOKEN_ARROW;
                    tok.len = 2;
                    p += 2;
                    break;
                } else if(*p == '-' && p[1] >= '0' && p[1] <= '9') {
                    char* end;
                    if(is_float(p)) {
                        tok.fval = strtod(p, &end);
                        tok.type = UNI_TOKEN_FLOAT;
                    } else {
                        tok.ival = strtoll(p, &end, 0);
                        tok.type = UNI_TOKEN_INT;
                    }
                    tok.len = (size_t)(end - p);
                    p = end;
                    break;
                }

                const char* start = p;
                while(
                    *p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' &&
                    *p != '(' && *p != ')' &&
                    *p != '{' && *p != '}' &&
                    *p != '\"' && *p != ':'
                ) {
                    p++;
                }

                tok.type = UNI_TOKEN_WORD;
                tok.start = start;
                tok.len = (size_t)(p - start);
            } break;
        }

        out[n++] = tok;
    }

    return n;
}

void uni_printToken(uniToken tok) {
    printf("TOKEN(\n");

    printf("    type = ");
    switch(tok.type) {
        case UNI_TOKEN_NULL:    printf("NULL\n");   break;
        case UNI_TOKEN_INT:     printf("INT\n");    break;
        case UNI_TOKEN_FLOAT:   printf("FLOAT\n");  break;
        case UNI_TOKEN_STRING:  printf("STRING\n"); break;
        case UNI_TOKEN_WORD:    printf("WORD\n");   break;
        case UNI_TOKEN_LPAREN:  printf("LPAREN\n"); break;
        case UNI_TOKEN_RPAREN:  printf("RPAREN\n"); break;
        case UNI_TOKEN_LBRACE:  printf("LBRACE\n"); break;
        case UNI_TOKEN_RBRACE:  printf("RBRACE\n"); break;
        case UNI_TOKEN_COLON:   printf("COLON\n");  break;
        case UNI_TOKEN_ARROW:   printf("ARROW\n");  break;
        case UNI_TOKEN_EOF:     printf("EOF\n");    break;
    }

    printf("    val = %.*s\n", (int)tok.len, tok.start);

    printf(")\n");
}

static bool is_float(const char* p) {
    if(*p == '-') p++;
    while(*p >= '0' && *p <= '9') p++;
    return *p == '.' || *p == 'e' || *p == 'E';
}
