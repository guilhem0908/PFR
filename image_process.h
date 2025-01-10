//
// Created by Guilhem & Alec on 09/01/2025.
//

#ifndef IMAGE_PROCESS_H
#define IMAGE_PROCESS_H

typedef struct {
    int width;
    int height;
    int** red_components;
    int** green_components;
    int** blue_components;
    int** quantized_pixels;
} ImageData_s;

typedef ImageData_s* ImageData;

/**
 * @brief Extracts pixel data for a single color component from image text.
 *
 * @param height Number of rows (image height).
 * @param width Number of columns (image width).
 * @param color_components Pointer to the 2D array to store the color component values.
 * @param cursor_image_text Pointer to the cursor pointing to the text data being parsed.
 */
void extract_color_components(int height, int width, int** color_components, char* cursor_image_text);

/**
 * @brief Extracts image data (dimensions and RGB pixels) from a image text file.
 *
 * @param path File path of the image text file.
 * @return A pointer to the allocated ImageData structure, or NULL on error.
 */
ImageData extract_image_text_data(const char* path);

/**
 * @brief Quantifies an RGB pixel by combining the n most significant bits of each color component (Red, Green, Blue).
 *
 * @param R Red component (integer between 0 and 255).
 * @param G Green component (integer between 0 and 255).
 * @param B Blue component (integer between 0 and 255).
 * @param n Number of significant bits to use for each component (1 ≤ n ≤ 8).
 * @return An integer ranging from 0 to 2^(3n) - 1, or -1 on error.
 */
int quantize_pixel(int R, int G, int B, int n);

void quantize_image(ImageData image);


#endif //IMAGE_PROCESS_H
