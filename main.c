#include "image_process.h"
#include <stdio.h>
#include <stdlib.h>
#include "file_operations.h"

int main(int argc, char *argv[]) {

    char path[1024];
    snprintf(path, sizeof(path), "%s", argv[1]);

    printf("Tentative d'ouverture du fichier : %s\n", path);
    FILE* file = fopen(path, "rb");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return -1;
    }

    const ImageData image_data = extract_image_text_data(path);

    Clusters clusters = find_clusters(image_data);
    update_binary_mask_with_largest_cluster(clusters);
    find_clusters_attributes(clusters);
    display_clusters(clusters);


    FILE *file2 = fopen("result.txt", "w");
    if (!file2) {
        perror("❌ Erreur lors de l'ouverture du fichier pour l'écriture.");
        return -1;
    }



    int cluster_count = number_clusters(clusters);
    fprintf(file2, "%d\n", cluster_count);

    Clusters current_cluster = clusters;
    while (current_cluster != NULL) {
        fprintf(file2, "%s %d %d %d\n", color_to_string(current_cluster->color),
                current_cluster->mid_x, current_cluster->mid_y, current_cluster->radius);
        current_cluster = current_cluster->next;
    }


    fclose(file2);

    free_clusters(clusters);
    free_image_data(image_data);

    return 0;
}
