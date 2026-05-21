#include "headers/args.h"

typedef enum uniFlagType_t {
    UNI_FLAG_STRING,
} uniFlagType;

typedef struct uniFlagDef_t {
    const char* name;
    uniFlagType type;
    void* target;
    const char* meta;
    const char* help;
} uniFlagDef;

#define UNI_FLAG_COUNT 1

static bool buildFlagTable(uniFlagDef* table, uniArgs* args) {
    table[0] = (uniFlagDef){
        "-o",
        UNI_FLAG_STRING,
        &args->output_file,
        "<file>",
        "Output file path"
    };

    return true;
}

static void printUsage(const char* prog) {
    fprintf(stderr, "Usage: %s [options] <input_file>\n\n", prog);
    fprintf(stderr, "Options:\n");

    uniArgs dummy = {0};
    uniFlagDef table[UNI_FLAG_COUNT];
    buildFlagTable(table, &dummy);

    for(size_t i = 0; i < UNI_FLAG_COUNT; i++) {
        fprintf(
            stderr, "  %-2s %-8s %s\n",
            table[i].name, table[i].meta, table[i].help
        );
    }
    fprintf(stderr, "\n");
}

bool uni_parseArgs(int argc, char** argv, uniArgs* out_args) {
    *out_args = (uniArgs){0};

    uniFlagDef table[UNI_FLAG_COUNT];
    buildFlagTable(table, out_args);

    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            bool matched = false;
            for(size_t f = 0; f < UNI_FLAG_COUNT; f++) {
                if(strcmp(argv[i], table[f].name) != 0) continue;
                else matched = true;

                switch(table[f].type) {
                    case UNI_FLAG_STRING: {
                        if(i + 1 >= argc) {
                            fprintf(stderr, "[ERROR] No argument supplied to '%s'\n\n", table[f].name);
                            printUsage(argv[0]);
                            return false;
                        }

                        *(const char**)table[f].target = argv[++i];
                    } break;
                }
                break;
            }

            if(!matched) {
                fprintf(stderr, "[ERROR] Unknown flag '%s'\n\n", argv[i]);
                printUsage(argv[0]);
                return false;
            }
        } else {
            if(out_args->input_file) {
                fprintf(stderr, "[ERROR] Unexpected argument '%s'\n\n", argv[i]);
                printUsage(argv[0]);
                return false;
            }
            out_args->input_file = argv[i];
        }
    }

    if(!out_args->input_file) {
        fprintf(stderr, "[ERROR] No input filed supplied...\n\n");
        printUsage(argv[0]);
        return false;
    }

    return true;
}
