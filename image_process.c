//
// Created by Guilhem & Alec on 09/01/2025.
//

#include "image_process.h"
#include "file_operations.h"
#include <stdio.h>
#include <stdlib.h>


void extract_RGB_components(const int height, const int width, int** RGB_components, char* cursor_image_text) {
    if (!RGB_components) {
        perror("❌ Error allocating memory for RGB components.");
        return;
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            RGB_components[i][j] = strtol(cursor_image_text, &cursor_image_text, 10);
        }
    }
}


ImageData extract_image_text_data(const char* path) {
    const char* image_text = read_file(path);

    char* cursor_image_text;
    const int width = strtol(image_text, &cursor_image_text, 10);
    const int height = strtol(cursor_image_text, &cursor_image_text, 10);
    const int channels = strtol(cursor_image_text, &cursor_image_text, 10);
    if (channels != 3) {
        fprintf(stderr, "⚠️ Only RGB images are supported.\n");
        return NULL;
    }

    ImageData_s* const image_data = malloc(sizeof(ImageData_s));
    if (!image_data) {
        perror("❌ Error allocating memory for image data.");
        return NULL;
    }
    image_data->width = width;
    image_data->height = height;
    image_data->red_components = malloc(height * sizeof(int*));
    image_data->green_components = malloc(height * sizeof(int*));
    image_data->blue_components = malloc(height * sizeof(int*));
    if (!image_data->red_components || !image_data->green_components || !image_data->blue_components) {
        perror("❌ Error allocating memory for rows (height) in the image.");
        free(image_data->red_components);
        free(image_data->green_components);
        free(image_data->blue_components);
        free(image_data);
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        image_data->red_components[i] = malloc(width * sizeof(int));
        image_data->green_components[i] = malloc(width * sizeof(int));
        image_data->blue_components[i] = malloc(width * sizeof(int));
        if (!image_data->red_components[i] || !image_data->green_components[i] || !image_data->blue_components[i]) {
            perror("❌ Error allocating memory for columns (width) in the image.");
            for (int j = 0; j <= i; j++) {
                free(image_data->red_components[j]);
                free(image_data->green_components[j]);
                free(image_data->blue_components[j]);
            }
            free(image_data->red_components);
            free(image_data->green_components);
            free(image_data->blue_components);
            free(image_data);
            return NULL;
        }
    }

    image_data->quantized_pixels = malloc(width * height * sizeof(int));
    if (!image_data->quantized_pixels) {
        perror("❌ Error allocating memory for quantized pixels.");
        free(image_data->quantized_pixels);
        return NULL;
    }

    extract_RGB_components(height, width, image_data->red_components, cursor_image_text);
    extract_RGB_components(height, width, image_data->green_components, cursor_image_text);
    extract_RGB_components(height, width, image_data->blue_components, cursor_image_text);

    return image_data;
}


int quantize_pixel(const int R, const int G, const int B, const int n) {
    if (n < 1 || n > 8) {
        fprintf(stderr, "⚠️ The parameter 'n' must be between 1 and 8 inclusive.\n");
        return -1;
    }

    const int mask = (1 << n) - 1;
    const int R_quantized = (R >> (8 - n)) & mask;
    const int G_quantized = (G >> (8 - n)) & mask;
    const int B_quantized = (B >> (8 - n)) & mask;

    return (R_quantized << (2 * n)) | (G_quantized << n) | B_quantized;
}
