#ifndef UNI_ARGS_H_INCLUDED_
#define UNI_ARGS_H_INCLUDED_

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "util.h"

typedef struct uniArgs_t {
    char* input_file;           // Required
    char* output_file;          // Optional
} uniArgs;

bool uni_parseArgs(int argc, char** argv, uniArgs* out_args);

#endif
