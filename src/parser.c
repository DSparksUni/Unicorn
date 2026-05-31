#include "headers/parser.h"

uniParser* uni_createParser(uniToken* tokens, size_t tokens_len) {
    uniParser* parser = malloc(sizeof(uniParser));
    if(!parser) return NULL;

    parser->tokens = tokens;
    parser->num_tokens = tokens_len;
    parser->cursor = 0;

    return parser;
}

void uni_destroyParser(uniParser* parser) {
    free(parser->tokens);
    free(parser);
}

uniToken uni_peekParser(uniParser* parser) {
    return parser->tokens[parser->cursor];
}

uniToken uni_advanceParser(uniParser* parser) {
    uniToken tok = parser->tokens[parser->cursor];
    if(tok.type != UNI_TOKEN_EOF) parser->cursor++;

    return tok;
}

bool uni_expectParser(uniParser* parser, uniTokenType type) {
    if(uni_peekParser(parser).type == type) {
        uni_advanceParser(parser);
        return true;
    }

    return false;
}

uniOp* uni_parseOne(uniParser* parser) {
    uniToken tok = uni_peekParser(parser);
    switch(tok.type) {
        case UNI_TOKEN_INT: {
            uni_advanceParser(parser);

            uniOp* op = malloc(sizeof(uniOp));
            if(!op) return NULL;

            op->type = UNI_OP_PUSH_INT;
            op->line = tok.line;
            op->ival = tok.ival;

            return op;
        } break;

        case UNI_TOKEN_STRING: {
            uni_advanceParser(parser);

            uniOp* op = malloc(sizeof(uniOp));
            if(!op) return NULL;

            op->type = UNI_OP_PUSH_STR;
            op->line = tok.line;
            op->sval.start = tok.start;
            op->sval.len = tok.len;

            return op;
        } break;

        case UNI_TOKEN_LBRACE: {
            uni_advanceParser(parser);
            return uni_parseBlock(parser);
        } break;

        case UNI_TOKEN_WORD: {
            uni_advanceParser(parser);

            if(tok.len == 2 && strncmp(tok.start, "if", 2) == 0) {
                if(uni_peekParser(parser).type != UNI_TOKEN_LBRACE) {
                    fprintf(
                        stderr, "[ERROR] (line %zu) 'if' must be followed by a block\n",
                        tok.line
                    );
                    return NULL;
                }
                uni_advanceParser(parser);

                uniOp* then_body = uni_parseBlock(parser);
                if(!then_body) return NULL;

                uniOp* else_body = NULL;
                uniToken next = uni_peekParser(parser);
                if(
                    next.type == UNI_TOKEN_WORD && next.len == 4 &&
                    strncmp(next.start, "else", 4) == 0
                ) {
                    uni_advanceParser(parser);
                    if(uni_peekParser(parser).type != UNI_TOKEN_LBRACE) {
                        fprintf(
                            stderr, "[ERROR] (line %zu) 'else' must be followed by a block\n",
                            next.line
                        );
                        uni_destroyOp(then_body);
                        return NULL;
                    }
                    uni_advanceParser(parser);

                    else_body = uni_parseBlock(parser);
                    if(!else_body) {
                        uni_destroyOp(then_body);
                        return NULL;
                    }
                }

                uniOp* op = malloc(sizeof(uniOp));
                if(!op) {
                    uni_destroyOp(then_body);
                    if(else_body) uni_destroyOp(else_body);
                    return NULL;
                }

                op->type = UNI_OP_IF;
                op->line = tok.line;
                op->cval.then_body = then_body;
                op->cval.else_body = else_body;

                return op;
            }

            if(tok.len == 5 && strncmp(tok.start, "while", 5) == 0) {
                if(uni_peekParser(parser).type != UNI_TOKEN_LBRACE) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) 'while' must be followed by a condition block\n",
                        tok.line
                    );
                    return NULL;
                }
                uni_advanceParser(parser);

                uniOp* cond_body = uni_parseBlock(parser);
                if(!cond_body) return NULL;

                if(uni_peekParser(parser).type != UNI_TOKEN_LBRACE) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) 'while' condition block must be followed by a body block\n",
                        tok.line
                    );
                    return NULL;
                }
                uni_advanceParser(parser);

                uniOp* loop_body = uni_parseBlock(parser);
                if(!loop_body) {
                    uni_destroyOp(cond_body);
                    return NULL;
                }

                uniOp* op = malloc(sizeof(uniOp));
                if(!op) {
                    uni_destroyOp(cond_body);
                    uni_destroyOp(loop_body);
                    return NULL;
                }

                op->type = UNI_OP_WHILE;
                op->line = tok.line;
                op->wval.cond_body = cond_body;
                op->wval.loop_body = loop_body;

                return op;
            }

            if(tok.len == 3 && strncmp(tok.start, "def", 3) == 0) {
                if(uni_peekParser(parser).type != UNI_TOKEN_WORD) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) 'def' must be followed by the word's name\n",
                        tok.line
                    );
                    return NULL;
                }

                uniToken name_tok = uni_advanceParser(parser);

                if(uni_peekParser(parser).type != UNI_TOKEN_LBRACE) {
                    fprintf(
                        stderr,
                        "[ERROR] (line %zu) 'def' names must be followed by a block\n",
                        tok.line
                    );
                    return NULL;
                }
                uni_advanceParser(parser);

                uniOp* body = uni_parseBlock(parser);
                if(!body) return NULL;

                uniOp* op = malloc(sizeof(uniOp));
                if(!op) {
                    uni_destroyOp(body);
                    return NULL;
                }

                op->type = UNI_OP_DEF;
                op->line = tok.line;
                op->dval.name = name_tok.start;
                op->dval.name_len = name_tok.len;
                op->dval.body = body;

                return op;
            }

            uniOp* op = malloc(sizeof(uniOp));
            if(!op) return NULL;

            op->type = UNI_OP_WORD;
            op->line = tok.line;
            op->sval.start = tok.start;
            op->sval.len = tok.len;

            return op;
        } break;

        default: return NULL;
    }
}

uniOp* uni_parseBlock(uniParser* parser) {
    uniOp* op = malloc(sizeof(uniOp));
    if(!op) return NULL;

    op->type = UNI_OP_BLOCK;
    op->bval.num_items = 0;

    size_t op_cap = 8;
    op->bval.items = malloc(op_cap * sizeof(uniOp*));
    if(!op->bval.items) return NULL;

    while(
        uni_peekParser(parser).type != UNI_TOKEN_RBRACE &&
        uni_peekParser(parser).type != UNI_TOKEN_EOF
    ) {
        uniOp* child = uni_parseOne(parser);
        if(!child) break;

        if(op->bval.num_items >= op_cap) {
            op_cap *= 2;
            op->bval.items = realloc(op->bval.items, op_cap * sizeof(uniOp*));
            if(!op->bval.items) break;
        }

        op->bval.items[op->bval.num_items++] = child;
    }

    uni_expectParser(parser, UNI_TOKEN_RBRACE);
    return op;
}

uniOp* uni_parseProgram(uniParser* parser) {
    uniOp* op = malloc(sizeof(uniOp));
    if(!op) return NULL;

    op->type = UNI_OP_BLOCK;
    op->bval.num_items = 0;

    size_t op_cap = 16;
    op->bval.items = malloc(op_cap * sizeof(uniOp*));
    if(!op->bval.items) return NULL;

    while(uni_peekParser(parser).type != UNI_TOKEN_EOF) {
        uniOp* child = uni_parseOne(parser);
        if(!child) break;

        if(op->bval.num_items >= op_cap) {
            op_cap *= 2;
            op->bval.items = realloc(op->bval.items, op_cap * sizeof(uniOp*));
            if(!op->bval.items) return NULL;
        }

        op->bval.items[op->bval.num_items++] = child;
    }

    return op;
}

void print_indent(size_t depth) {
    for(size_t _ = 0; _ < depth; _++) printf(" ");
}

void uni_printOp(uniOp* op, size_t indent) {
    print_indent(indent);
    switch(op->type) {
        case UNI_OP_PUSH_INT: {
            printf("PUSH_INT %lld\n", op->ival);
        } break;

        case UNI_OP_PUSH_STR: {
            printf("PUSH_STR \"%.*s\"\n", (int)op->sval.len, op->sval.start);
        } break;

        case UNI_OP_WORD: {
            printf("WORD \"%.*s\"\n", (int)op->sval.len, op->sval.start);
        } break;

        case UNI_OP_BLOCK: {
            printf("BLOCK (length %zu)\n", op->bval.num_items);
            for(size_t i = 0; i < op->bval.num_items; i++) {
                uni_printOp(op->bval.items[i], indent+4);
            }
        } break;

        case UNI_OP_IF: {
            printf("IF\n");

            print_indent(indent);
            printf("    THEN:\n");
            for(size_t i = 0; i < op->cval.then_body->bval.num_items; i++) {
                uni_printOp(op->cval.then_body->bval.items[i], indent+4);
            }

            if(op->cval.else_body) {
                print_indent(indent);
                printf("    ELSE:\n");
                for(size_t i = 0; i < op->cval.else_body->bval.num_items; i++) {
                    uni_printOp(op->cval.else_body->bval.items[i], indent+4);
                }
            }
        } break;

        case UNI_OP_WHILE: {
            printf("WHILE\n");

            print_indent(indent);
            printf("    COND:\n");
            for(size_t i = 0; i < op->wval.cond_body->bval.num_items; i++) {
                uni_printOp(op->wval.cond_body->bval.items[i], indent+4);
            }

            printf("    LOOP:\n");
            for(size_t i = 0; i < op->wval.loop_body->bval.num_items; i++) {
                uni_printOp(op->wval.loop_body->bval.items[i], indent+4);
            }
        } break;

        case UNI_OP_DEF: {
            printf("DEF (%.*s)\n", (int)op->dval.name_len, op->dval.name);
            for(size_t i = 0; i < op->dval.body->bval.num_items; i++) {
                uni_printOp(op->dval.body->bval.items[i], indent+4);
            }
        } break;
    }
}

void uni_destroyOp(uniOp* op) {
    if(op->type == UNI_OP_BLOCK) {
        for(size_t i = 0; i < op->bval.num_items; i++)
            uni_destroyOp(op->bval.items[i]);
        free(op->bval.items);
    } else if(op->type == UNI_OP_IF) {
        uni_destroyOp(op->cval.then_body);
        if(op->cval.else_body) uni_destroyOp(op->cval.else_body);
    } else if(op->type == UNI_OP_WHILE) {
        uni_destroyOp(op->wval.cond_body);
        uni_destroyOp(op->wval.loop_body);
    } else if(op->type == UNI_OP_DEF) {
        uni_destroyOp(op->dval.body);
    }

    free(op);
}
