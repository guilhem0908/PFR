//
// Created by Guilhem on 25/01/2025.
//

#include <stdio.h>
#include "cluster.h"

#include <stdlib.h>

Clusters init_clusters(void) {
    return NULL;
}

Clusters add_cluster(const Clusters clusters, const int width, const int height, const int number_pixels,
                    const int** binary_mask, const Color color) {
    const Clusters new_clusters = malloc(sizeof(Cluster));
    if (!new_clusters) {
        perror("❌ Error allocating memory for clusters.");
        return NULL;
    }

    new_clusters->width = width;
    new_clusters->height = height;
    new_clusters->number_pixels = number_pixels;
    new_clusters->color = color;
    new_clusters->mid_x = -1;
    new_clusters->mid_y = -1;
    new_clusters->radius = -1;

    new_clusters->binary_mask = malloc(height * sizeof(int*));
    if (new_clusters->binary_mask == NULL) {
        perror("❌ Error allocating memory for binary_mask rows.");
        free(new_clusters);
        return NULL;
    }
    for (int i = 0; i < height; ++i) {
        new_clusters->binary_mask[i] = malloc(width * sizeof(int));
        if (new_clusters->binary_mask[i] == NULL) {
            for (int j = 0; j < i; ++j) {
                free(new_clusters->binary_mask[j]);
            }
            free(new_clusters->binary_mask);
            free(new_clusters);
            return NULL;
        }
    }

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            new_clusters->binary_mask[i][j] = binary_mask[i][j];
        }
    }

    new_clusters->next = clusters;
    return new_clusters;
}

char* color_to_string(const Color color) {
    switch (color) {
        case ORANGE: return "orange";
        case BLUE: return "blue";
        case YELLOW: return "jaune";
        default: return NULL;
    }
}

int number_clusters(const Clusters clusters) {
    int number_clusters = 0;
    Clusters current = clusters;
    while (current != NULL) {
        number_clusters++;
        current = current->next;
    }
    return number_clusters;
}

void find_clusters_attributes(const Clusters clusters) {
    Clusters current = clusters;
    while (current != NULL) {
        int min_x = current->height;
        int min_y = current->width;
        int max_x = 0;
        int max_y = 0;
        for (int i = 0; i < current->height; i++){
            for(int j = 0; j < current->width; j++){
                if (current->binary_mask[i][j] == 1 && min_y > i){
                    min_y = i;
                }
                if (current->binary_mask[i][j] == 1){
                    max_y = i;
                }
                if (current->binary_mask[i][j] == 1 && min_x > j){
                    min_x = j ;
                }
                if (current->binary_mask[i][j] == 1){
                    max_x = j;
                }
            }
        }
        current->mid_x = min_x + (max_x - min_x) / 2;
        current->mid_y = min_y + (max_y - min_y) / 2;
        const int radius_x = (max_x - min_x) / 2;
        const int radius_y = (max_y - min_y) / 2;
        current->radius = (radius_x > radius_y) ? radius_x : radius_y;

        current = current->next;
    }
}

void display_clusters(const Clusters clusters) {
    Clusters current = clusters;
    while (current != NULL) {
        printf("Balle de couleur %s detecte.\n", color_to_string(current->color));
        printf("Positionne en (%d, %d) et de rayon %d.\n", current->mid_x, current->mid_y, current->radius);
        printf("\n");
        current = current->next;
    }
}

void free_clusters(const Clusters clusters) {
    Clusters current = clusters;
    while (current != NULL) {
        const Clusters temp = current;
        current = current->next;

        for (int i = 0; i < temp->height; i++) {
            if (temp->binary_mask[i] != NULL)
                free(temp->binary_mask[i]);
        }
        if (temp->binary_mask != NULL)
            free(temp->binary_mask);
        free(temp);
    }
}