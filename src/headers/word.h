#ifndef UNI_WORD_H_INCLUDED_
#define UNI_WORD_H_INCLUDED_

#include <string.h>

#include "parser.h"

struct uniEmitter_t;

typedef enum uniTypeKind_t {
    UNI_KIND_INT,
    UNI_KIND_STRING,
    UNI_KIND_VAR
} uniTypeKind;

typedef struct uniType_t {
    uniTypeKind kind;
    uint8_t var_id;
} uniType;

#define UNI_TYPE_INT ((uniType){UNI_KIND_INT, 0})
#define UNI_TYPE_STRING ((uniType){UNI_KIND_STRING, 0})
#define UNI_TYPE_VAR(id) ((uniType){UNI_KIND_VAR, (id)})

typedef struct uniWord_t {
    char* name;
    uniType* inputs;    // Types popped by word (from bottom to top of stack)
    size_t num_inputs;
    uniType* outputs;   // Types pushed by word
    size_t num_outputs;
    void (*emit)(struct uniEmitter_t* emitter);
    uniOp* body;
} uniWord;

void uni_registerWord(uniWord word);
uniWord* uni_lookupWord(const char* name, size_t len);

void uni_cleanupWords(void);

#endif
