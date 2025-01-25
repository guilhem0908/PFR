#include "image_process.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <nom_du_fichier>\n", argv[0]);
        return 1;
    }

    char path[50];
    snprintf(path, sizeof(path), "../IMG_300/%s.txt", argv[1]);

    const ImageData image_data = extract_image_text_data(path);

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
