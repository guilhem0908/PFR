#include "image_process.h"
#include <stdio.h>
#include <stdlib.h>


int main() {
    const ImageData image_data = extract_image_text_data("../IMG_300/IMG_5392.txt");

    const Clusters clusters = find_clusters(image_data);
    find_clusters_attributes(clusters);
    display_clusters(clusters);

    free_clusters(clusters);
    free_image_data(image_data);

    return 0;
}
