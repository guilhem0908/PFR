//
// Created by Guilhem & Alec on 09/01/2025.
//

#include "image_process.h"
#include "file_operations.h"
#include <stdio.h>
#include <stdlib.h>


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
    image_data->red_pixels = malloc(height * sizeof(int*));
    image_data->green_pixels = malloc(height * sizeof(int*));
    image_data->blue_pixels = malloc(height * sizeof(int*));
    if (!image_data->red_pixels || !image_data->green_pixels || !image_data->blue_pixels) {
        perror("❌ Error allocating memory for rows (height) in the image.");
        free(image_data->red_pixels);
        free(image_data->green_pixels);
        free(image_data->blue_pixels);
        free(image_data);
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        image_data->red_pixels[i] = malloc(width * sizeof(int));
        image_data->green_pixels[i] = malloc(width * sizeof(int));
        image_data->blue_pixels[i] = malloc(width * sizeof(int));
        if (!image_data->red_pixels[i] || !image_data->green_pixels[i] || !image_data->blue_pixels[i]) {
            perror("❌ Error allocating memory for columns (width) in the image.");
            for (int j = 0; j <= i; j++) {
                free(image_data->red_pixels[j]);
                free(image_data->green_pixels[j]);
                free(image_data->blue_pixels[j]);
            }
            free(image_data->red_pixels);
            free(image_data->green_pixels);
            free(image_data->blue_pixels);
            free(image_data);
            return NULL;
        }
    }
    return image_data;
}

int quantize_pixel(const int R, const int G, const int B, const int n) {
    if (n < 1 || n > 8) {
        fprintf(stderr, "⚠️ Error: The parameter 'n' must be between 1 and 8 inclusive.\n");
        return -1;
    }

    const int mask = (1 << n) - 1;
    const int R_quantized = (R >> (8 - n)) & mask;
    const int G_quantized = (G >> (8 - n)) & mask;
    const int B_quantized = (B >> (8 - n)) & mask;

    return (R_quantized << (2 * n)) | (G_quantized << n) | B_quantized;
}
