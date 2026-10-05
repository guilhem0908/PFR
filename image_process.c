//
// Created by Guilhem & Alec on 09/01/2025.
//

#include "image_process.h"
#include "file_operations.h"
#include <stdio.h>
#include <stdlib.h>


void extract_color_components(const int height, const int width, int** color_components, char** cursor_image_text) {
    if (!color_components) {
        perror("❌ Error allocating memory for color components.");
        return;
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            color_components[i][j] = strtol(*cursor_image_text, cursor_image_text, 10);
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

    const ImageData image_data = malloc(sizeof(ImageData_s));
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
        perror("❌ Error allocating memory for rows (height) in the RGB components.");
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
            perror("❌ Error allocating memory for columns (width) in the RGB components.");
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

    image_data->quantized_pixels = malloc(height * sizeof(int*));
    if (!image_data->quantized_pixels) {
        perror("❌ Error allocating memory for rows (height) in the quantized pixels.");
        free(image_data->quantized_pixels);
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        image_data->quantized_pixels[i] = malloc(height * sizeof(int));
        if (!image_data->quantized_pixels[i]) {
            perror("❌ Error allocating memory for columns (width) in the RGB components.");
            for (int j = 0; j <= i; j++) {
                free(image_data->quantized_pixels[i]);
            }
            free(image_data->quantized_pixels);
            return NULL;
        }
    }

    extract_color_components(height, width, image_data->red_components, &cursor_image_text);
    extract_color_components(height, width, image_data->green_components, &cursor_image_text);
    extract_color_components(height, width, image_data->blue_components, &cursor_image_text);

    return image_data;
}


int quantize_pixel(const int R, const int G, const int B, const int n) {
    if (n < 1 || n > 8) {
        fprintf(stderr, "⚠️ The parameter 'n' must be between 1 and 8 inclusive.\n");
        return -1;
    }

    const int mask = (1 << n) - 1;
    const int R_quantized = R >> (8 - n) & mask;
    const int G_quantized = G >> (8 - n) & mask;
    const int B_quantized = B >> (8 - n) & mask;

    return R_quantized << 2 * n | G_quantized << n | B_quantized;
}

void quantize_image(const ImageData image, const int n) {
    image->n = n;
    for (int i=0; i < image->height; i++){
        for (int j=0; j < image->width; j++){
            const int R = image->red_components[i][j];
            const int G = image->green_components[i][j];
            const int B = image->blue_components[i][j];
            image->quantized_pixels[i][j] = quantize_pixel(R, G, B, n);
        }
    }
}

void get_thresholds(const Color color, int thresholds[6]) {
    switch (color) {
        case ORANGE:
            thresholds[0] = 92; thresholds[1] = 250; // Red Min/Max
            thresholds[2] = 22; thresholds[3] = 70; // Green Min/Max
            thresholds[4] = 2; thresholds[5] = 60; // Blue Min/Max
        break;
        case BLUE:
            thresholds[0] = 3; thresholds[1] = 40; // Red Min/Max
            thresholds[2] = 30; thresholds[3] = 100; // Green Min/Max
            thresholds[4] = 68; thresholds[5] = 200; // Blue Min/Max
        break;
        case YELLOW:
            thresholds[0] = 150; thresholds[1] = 255; // Red Min/Max
            thresholds[2] = 160; thresholds[3] = 255; // Green Min/Max
            thresholds[4] = 13; thresholds[5] = 90; // Blue Min/Max
        break;
        default:
            ;
    }
}

Clusters find_clusters(const ImageData image) {
    Clusters clusters = init_clusters();
    for (Color color = ORANGE; color <= YELLOW; color++) {
        int thresholds[6];
        get_thresholds(color, thresholds);
        int number_pixels = 0;

        int** binary_mask = malloc(image->height * sizeof(int*));
        if (!binary_mask) {
            perror("❌ Error allocating memory for rows (height) in the binary_mask.");
            free(binary_mask);
            return NULL;
        }
        for (int i = 0; i < image->height; i++) {
            binary_mask[i] = malloc(image->width * sizeof(int));
            if (!binary_mask[i]) {
                perror("❌ Error allocating memory for columns (width) in the binary_mask.");
                for (int j = 0; j <= i; j++) {
                    free(binary_mask[i]);
                }
                free(binary_mask);
                return NULL;
            }
        }
        for (int i = 0; i < image->height; ++i) {
            for (int j = 0; j < image->width; ++j) {
			binary_mask[i][j] =0;
              if (image->red_components[i][j] >= thresholds[0] && image->red_components[i][j] <= thresholds[1] &&
                    image->green_components[i][j] >= thresholds[2] && image->green_components[i][j] <= thresholds[3] &&
                    image->blue_components[i][j] >= thresholds[4] && image->blue_components[i][j] <= thresholds[5]) {
                    binary_mask[i][j] = 1;
                    number_pixels++;
                }
            }
        }

        if (number_pixels > 0 ) {
            clusters = add_cluster(clusters, image->width, image->height, number_pixels, binary_mask, color);
            if (!clusters) {
                perror("❌ Error allocating memory for clusters. (2)");
                for (int i = 0; i <= image->height; i++) {
                    free(binary_mask[i]);
                }
                free(binary_mask);
                return NULL;
            }
        }
        for (int i = 0; i < image->height; i++) {
            free(binary_mask[i]);
        }
        free(binary_mask);
    }
    return clusters;
}

void free_image_data(const ImageData image) {
    if (image == NULL) {
        return;
    }

    for (int i = 0; i < image->height; ++i) {
        free(image->red_components[i]);
    }
    free(image->red_components);

    for (int i = 0; i < image->height; ++i) {
        free(image->green_components[i]);
    }
    free(image->green_components);

    for (int i = 0; i < image->height; ++i) {
        free(image->blue_components[i]);
    }
    free(image->blue_components);

    for (int i = 0; i < image->height; ++i) {
        free(image->quantized_pixels[i]);
    }
    free(image->quantized_pixels);

    free(image);
}
