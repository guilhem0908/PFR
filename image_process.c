//
// Created by Guilhem & Alec on 09/01/2025.
//

#include "image_process.h"
#include "file_operations.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


/* Largest accepted image side; keeps every allocation and index well inside the range of int. */
#define MAX_IMAGE_SIDE 4096

/* Reads the next integer of the text and moves the cursor past it. Returns false when there is none. */
static bool read_integer(const char** cursor_image_text, long* value) {
    char* end;
    errno = 0;
    *value = strtol(*cursor_image_text, &end, 10);
    if (end == *cursor_image_text || errno == ERANGE) {
        return false;
    }
    *cursor_image_text = end;
    return true;
}

/* Fills one colour plane from the text. Returns false on a missing sample or a value outside 0-255. */
static bool extract_color_components(const int height, const int width, int** color_components,
                                     const char** cursor_image_text) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            long value;
            if (!read_integer(cursor_image_text, &value) || value < 0 || value > 255) {
                return false;
            }
            color_components[i][j] = (int) value;
        }
    }
    return true;
}

/* Allocates a height x width plane filled with zeros, or returns NULL. */
static int** allocate_plane(const int height, const int width) {
    int** plane = malloc(height * sizeof(int*));
    if (!plane) {
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        plane[i] = calloc(width, sizeof(int));
        if (!plane[i]) {
            for (int j = 0; j < i; j++) {
                free(plane[j]);
            }
            free(plane);
            return NULL;
        }
    }
    return plane;
}

static void free_plane(int** plane, const int height) {
    if (plane == NULL) {
        return;
    }
    for (int i = 0; i < height; i++) {
        free(plane[i]);
    }
    free(plane);
}

ImageData create_image_data(const int width, const int height) {
    if (width < 1 || height < 1 || width > MAX_IMAGE_SIDE || height > MAX_IMAGE_SIDE) {
        fprintf(stderr, "Unsupported image size %d x %d (each side must be between 1 and %d).\n",
                width, height, MAX_IMAGE_SIDE);
        return NULL;
    }

    const ImageData image_data = malloc(sizeof(ImageData_s));
    if (!image_data) {
        perror("Error allocating memory for image data");
        return NULL;
    }
    image_data->width = width;
    image_data->height = height;
    image_data->n = 0;
    image_data->quantized_pixels = NULL;
    image_data->red_components = allocate_plane(height, width);
    image_data->green_components = allocate_plane(height, width);
    image_data->blue_components = allocate_plane(height, width);
    if (!image_data->red_components || !image_data->green_components || !image_data->blue_components) {
        perror("Error allocating memory for the RGB components");
        free_image_data(image_data);
        return NULL;
    }
    return image_data;
}

ImageData parse_image_text(const char* image_text) {
    const char* cursor_image_text = image_text;
    long width, height, channels;
    if (!read_integer(&cursor_image_text, &width) || !read_integer(&cursor_image_text, &height) ||
        !read_integer(&cursor_image_text, &channels)) {
        fprintf(stderr, "Invalid image header: expected 'width height channels'.\n");
        return NULL;
    }
    if (channels != 3) {
        fprintf(stderr, "Only RGB images are supported (3 channels, found %ld).\n", channels);
        return NULL;
    }
    if (width < 1 || height < 1 || width > MAX_IMAGE_SIDE || height > MAX_IMAGE_SIDE) {
        fprintf(stderr, "Unsupported image size %ld x %ld (each side must be between 1 and %d).\n",
                width, height, MAX_IMAGE_SIDE);
        return NULL;
    }

    const ImageData image_data = create_image_data((int) width, (int) height);
    if (!image_data) {
        return NULL;
    }

    if (!extract_color_components(image_data->height, image_data->width, image_data->red_components, &cursor_image_text) ||
        !extract_color_components(image_data->height, image_data->width, image_data->green_components, &cursor_image_text) ||
        !extract_color_components(image_data->height, image_data->width, image_data->blue_components, &cursor_image_text)) {
        fprintf(stderr, "Invalid image data: expected 3 x %ld x %ld integers between 0 and 255.\n", height, width);
        free_image_data(image_data);
        return NULL;
    }

    return image_data;
}

ImageData extract_image_text_data(const char* path) {
    char* image_text = read_file(path);
    if (!image_text) {
        return NULL;
    }

    const ImageData image_data = parse_image_text(image_text);
    free(image_text);
    return image_data;
}


int quantize_pixel(const int R, const int G, const int B, const int n) {
    if (n < 1 || n > 8) {
        fprintf(stderr, "The parameter 'n' must be between 1 and 8 inclusive.\n");
        return -1;
    }

    const int mask = (1 << n) - 1;
    const int R_quantized = R >> (8 - n) & mask;
    const int G_quantized = G >> (8 - n) & mask;
    const int B_quantized = B >> (8 - n) & mask;

    return R_quantized << 2 * n | G_quantized << n | B_quantized;
}

bool quantize_image(const ImageData image, const int n) {
    if (n < 1 || n > 8) {
        fprintf(stderr, "The parameter 'n' must be between 1 and 8 inclusive.\n");
        return false;
    }
    if (image->quantized_pixels == NULL) {
        image->quantized_pixels = allocate_plane(image->height, image->width);
        if (image->quantized_pixels == NULL) {
            perror("Error allocating memory for the quantized pixels");
            return false;
        }
    }

    image->n = n;
    for (int i=0; i < image->height; i++){
        for (int j=0; j < image->width; j++){
            const int R = image->red_components[i][j];
            const int G = image->green_components[i][j];
            const int B = image->blue_components[i][j];
            image->quantized_pixels[i][j] = quantize_pixel(R, G, B, n);
        }
    }
    return true;
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

    free_plane(image->red_components, image->height);
    free_plane(image->green_components, image->height);
    free_plane(image->blue_components, image->height);
    free_plane(image->quantized_pixels, image->height);
    free(image);
}
