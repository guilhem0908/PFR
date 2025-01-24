#include "image_process.h"
#include <stdio.h>


int main() {
    const ImageData image_data = extract_image_text_data("../IMG_300/IMG_5390.txt");

    quantize_image(image_data, 2);

    FILE* file = fopen("", "w");
    if (file == NULL) {
        perror("Error opening file");
    }

    for (int i = 0; i < image_data->height; i++) {
        for (int j = 0; j < image_data->width; j++) {
            fprintf(file, "%d ", image_data->quantized_pixels[i][j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);

    return 0;
}