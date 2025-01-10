//
// Created by Guilhem & Alec on 09/01/2025.
//

#ifndef IMAGE_PROCESS_H
#define IMAGE_PROCESS_H

typedef struct {
    int width;
    int height;
    int** red_pixels;
    int** green_pixels;
    int** blue_pixels;
    int** quantized_pixels;
} ImageData_s;

typedef ImageData_s* ImageData;

/**
 * @brief Extracts image data (dimensions and RGB pixels) from a image text file.
 *
 * @param path File path of the image text file.
 * @return A pointer to the allocated ImageData structure, or NULL on error.
 */
ImageData extract_image_text_data(const char* path);

/**
 * @brief Quantize a pixel's RGB values to a specified number of color levels.
 *
 * This function takes the RGB values of a pixel and reduces each component
 * (Red, Green, Blue) to a discrete set of values based on the given number of
 * levels. The quantization maps the color values to a set of `n` levels.
 *
 * @param R The red component of the pixel (0-255).
 * @param G The green component of the pixel (0-255).
 * @param B The blue component of the pixel (0-255).
 * @param n The number of color levels to quantize each component (typically a power of 2).
 * @return The quantized color value as an integer with each component in the lower `n` levels.
 *         The format is 0xRRGGBB, where each component is quantized.
 */
int quantize_pixel(const int R, const int G, const int B, const int n);

void quantize_image(ImageData image);


#endif //IMAGE_PROCESS_H
