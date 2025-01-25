#include "image_process.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[]) {


    char path[50];
    snprintf(path, sizeof(path), "C:/Users/Alec/CLionProjects/PFR/IMG_300/IMG_5389.txt");

    const ImageData image_data = extract_image_text_data("C:/Users/Alec/CLionProjects/PFR/IMG_300/IMG_5389.txt");

    Clusters clusters = find_clusters(image_data);
    find_clusters_attributes(clusters);
    display_clusters(clusters);

    FILE* file = fopen("../result.txt", "w");
    if (!file) {
        perror("❌ Error opening file.");
        return -1;
    }

    fprintf(file, "%d\n", number_clusters(clusters));
    while (clusters != NULL) {
        fprintf(file, "%s %d %d %d\n", color_to_string(clusters->color), clusters->mid_x, clusters->mid_y,
            clusters->radius);
        clusters = clusters->next;
    }
    fclose(file);

    free_clusters(clusters);
    free_image_data(image_data);

    return 0;
}
