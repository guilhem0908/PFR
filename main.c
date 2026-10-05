#include "image_process.h"
#include <stdio.h>
#include <stdlib.h>

#define RESULT_PATH "result.txt"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image.txt>\n", argc > 0 ? argv[0] : "PFR");
        return 2;
    }

    const ImageData image_data = extract_image_text_data(argv[1]);
    if (!image_data) {
        fprintf(stderr, "Could not load the image '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }

    Clusters clusters;
    if (!find_clusters(image_data, &clusters)) {
        free_image_data(image_data);
        return EXIT_FAILURE;
    }
    update_binary_mask_with_largest_cluster(clusters);
    clusters = find_clusters_attributes(clusters);
    display_clusters(clusters);

    FILE* result_file = fopen(RESULT_PATH, "w");
    if (!result_file) {
        perror("Error opening " RESULT_PATH);
        free_clusters(clusters);
        free_image_data(image_data);
        return EXIT_FAILURE;
    }

    fprintf(result_file, "%d\n", number_clusters(clusters));
    for (Clusters current_cluster = clusters; current_cluster != NULL; current_cluster = current_cluster->next) {
        fprintf(result_file, "%s %d %d %d\n", color_to_string(current_cluster->color),
                current_cluster->mid_x, current_cluster->mid_y, current_cluster->radius);
    }
    fclose(result_file);

    free_clusters(clusters);
    free_image_data(image_data);

    return EXIT_SUCCESS;
}
