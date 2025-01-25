//
// Created by Guilhem on 09/01/2025.
//

#include "file_operations.h"
#include <stdio.h>
#include <stdlib.h>

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        perror("❌ Error opening file.");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    const long file_length = ftell(file);
    rewind(file);

    char* buffer = malloc((file_length + 1) * sizeof(char));
    if (!buffer) {
        perror("❌ Error allocating memory for buffer.");
        fclose(file);
        return NULL;
    }

    const size_t elements_read = fread(buffer, 1, file_length, file);
    if (elements_read != file_length) {
        perror("❌ Error reading file.");
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
        perror("❌ Error opening file.");
        return false;
    }

    fprintf(file, "%s", text);
    fclose(file);
    return true;
}