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

int quantize_pixel(const int R, const int G, const int B, const int n);

void quantize_image(ImageData image);


#endif //IMAGE_PROCESS_H
