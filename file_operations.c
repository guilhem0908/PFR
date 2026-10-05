//
// Created by Guilhem on 09/01/2025.
//

#include "file_operations.h"
#include <stdio.h>
#include <stdlib.h>

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    long file_length = -1;
    if (fseek(file, 0, SEEK_END) == 0) {
        file_length = ftell(file);
    }
    if (file_length < 0) {
        perror("Error measuring file");
        fclose(file);
        return NULL;
    }
    rewind(file);

    char* buffer = malloc((size_t) file_length + 1);
    if (!buffer) {
        perror("Error allocating memory for buffer");
        fclose(file);
        return NULL;
    }

    const size_t elements_read = fread(buffer, 1, (size_t) file_length, file);
    if (elements_read != (size_t) file_length) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_length] = '\0';

    fclose(file);
    return buffer;
}

bool write_to_file(const char* path, const char* text) {
    FILE* file = fopen(path, "w");
    if (!file) {
        perror("Error opening file");
        return false;
    }

    fprintf(file, "%s", text);
    fclose(file);
    return true;
}
