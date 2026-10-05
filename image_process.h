//
// Created by Guilhem & Alec on 09/01/2025.
//

#ifndef IMAGE_PROCESS_H
#define IMAGE_PROCESS_H
#include <stdbool.h>
#include "cluster.h"

typedef struct {
    int width;
    int height;
    int** red_components;
    int** green_components;
    int** blue_components;
    int n;                  // bits kept per component by the last 'quantize_image' call, 0 before
    int** quantized_pixels; // NULL until 'quantize_image' is called
} ImageData_s;

typedef ImageData_s* ImageData;


/**
 * @brief Allocates an image of the given size with every RGB component set to 0.
 *
 * @param width Number of columns (image width), between 1 and 4096.
 * @param height Number of rows (image height), between 1 and 4096.
 * @return A pointer to the allocated ImageData structure, or NULL on error.
 *         The caller must release it with 'free_image_data'.
 */
ImageData create_image_data(int width, int height);

/**
 * @brief Parses an image from its text form: a header "width height channels"
 *        followed by the red, green and blue planes, each as height x width
 *        integers between 0 and 255 separated by whitespace.
 *
 * @param image_text Null-terminated text of the image.
 * @return A pointer to the allocated ImageData structure, or NULL when the
 *         header is invalid, the image is not RGB, or samples are missing or
 *         out of range.
 */
ImageData parse_image_text(const char* image_text);

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
 * @brief Quantize the RGB values of all pixels in an image.
 *
 * The quantized values are stored in `image->quantized_pixels`, allocated on first use.
 *
 * @param image Pointer to the `ImageData` structure containing the image data.
 * @param n Number of significant bits to keep for each component (1 ≤ n ≤ 8).
 * @return true on success, false if `n` is out of range or memory cannot be allocated.
 */
bool quantize_image(ImageData image, int n);

/**
 * @brief Retrieves the threshold values for a specific color.
 *
 * @param color The target color (defined in the Color enum).
 * @param thresholds Pointer to an array of 6 integers to store the thresholds:
 *        thresholds[0-1] for red min/max, thresholds[2-3] for green min/max, and thresholds[4-5] for blue min/max.
 */
void get_thresholds(Color color, int thresholds[6]);

/**
 * @brief Identifies and extracts clusters of pixels in an image based on predefined color thresholds.
 *
 * One cluster is created per colour that has at least one matching pixel; its
 * binary mask marks every pixel inside the colour thresholds.
 *
 * @param image Pointer to the `ImageData` structure containing the image data.
 * @param clusters Receives the head of the list (NULL when no pixel matches any colour).
 * @return true on success, false if memory cannot be allocated (the list is then NULL).
 */
bool find_clusters(const ImageData image, Clusters* clusters);

/**
 * @brief Releases an image and all its planes. Accepts NULL.
 *
 * @param image Pointer to the `ImageData` structure to free.
 */
void free_image_data(const ImageData image);

#endif //IMAGE_PROCESS_H