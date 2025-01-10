#include "file_operations.h"
#include "image_process.h"


int main() {
    const ImageData image_data = extract_image_text_data("../IMG_300/IMG_5390.txt");

    quantize_image(image_data, 2);

    write_to_file("../histogram.txt", image_data->quantized_pixels);

    return 0;
}