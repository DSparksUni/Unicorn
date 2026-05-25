#ifndef UNI_WORD_H_INCLUDED_
#define UNI_WORD_H_INCLUDED_

#include <string.h>

#include "parser.h"

struct uniEmitter_t;

typedef enum uniType_t {
    UNI_TYPE_INT,
    UNI_TYPE_STRING
} uniType;

typedef struct uniWord_t {
    const char* name;
    uniType* inputs;    // Types popped by word (from bottom to top of stack)
    size_t num_inputs;
    uniType* outputs;   // Types pushed by word
    size_t num_outputs;
    void (*emit)(struct uniEmitter_t* emitter);
} uniWord;

uniWord* uni_lookupWord(const char* name, size_t len);

#endif
