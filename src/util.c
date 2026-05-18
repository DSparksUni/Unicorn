#include "headers/util.h"

char* uni_readFile(const char* file_path, size_t* out_size) {
    FILE* fptr = fopen(file_path, "r");
    if(!fptr) return NULL;

    fseek(fptr, 0, SEEK_END);
    size_t file_size = (size_t)ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    char* content_buffer = malloc(file_size + 1);
    if(!content_buffer) return NULL;

    fread(content_buffer, 1, file_size, fptr);
    if(ferror(fptr)) return NULL;
    content_buffer[file_size] = '\0';

    fclose(fptr);
    *out_size = file_size;
    return content_buffer;
}
