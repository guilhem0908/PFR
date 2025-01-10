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
 * @brief Extracts pixel data for a single RGB component from image text.
 *
 * @param height Number of rows (image height).
 * @param width Number of columns (image width).
 * @param RGB_components Pointer to the 2D array to store the RGB component values.
 * @param cursor_image_text Pointer to the cursor pointing to the image text.
 */
void extract_RGB_components(int height, int width, int** RGB_components, char* cursor_image_text);

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

/**
 * @brief Quantizes the RGB values of all pixels in an image.
 *
 * This function iterates through each pixel in the image, quantizes its RGB
 * components using the specified number of quantization levels (`n`),
 * and stores the result in the `quantized_pixels` array.
 *
 * For each pixel, the red, green, and blue components are extracted,
 * quantized using the `quantize_pixel` function, and the result is stored
 * in the corresponding position in the `quantized_pixels` array.
 *
 * @param image Pointer to the `ImageData` structure containing the image data.
 * @param n The number of quantization levels for each component (1 ≤ n ≤ 8).
 */
void quantize_image(ImageData image, int n);


#endif //IMAGE_PROCESS_H