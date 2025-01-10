//
// Created by Guilhem on 09/01/2025.
//

#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdbool.h>

/**
 * @brief Reads a text file into a dynamically allocated string.
 * The string is null-terminated to ensure compatibility with C string functions.
 *
 * @param path File path to read from.
 * @return Pointer to the file's content as a string, or NULL on error.
 *         The caller must free the allocated memory using 'free'.
 */
char* read_file(const char* path);

/**
 * @brief Writes a string to a text file, overwriting its content if it already exists.
 *
 * @param path File path to write to.
 * @param text The string to write into the file.
 * @return true on success, false if file cannot be opened.
 */
bool write_to_file(const char* path, const char* text);

#endif //FILE_OPERATIONS_H
