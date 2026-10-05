//
// Created by Guilhem on 25/01/2025.
//

#include <stdio.h>
#include "cluster.h"

#include <stdlib.h>
int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

Clusters init_clusters(void) {
    return NULL;
}

Clusters add_cluster(const Clusters clusters, const int width, const int height, const int number_pixels, int** binary_mask, const Color color) {
    const Clusters new_clusters = malloc(sizeof(Cluster));
    if (!new_clusters) {
        perror("Error allocating memory for clusters");
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
        perror("Error allocating memory for binary_mask rows");
        free(new_clusters);
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        new_clusters->binary_mask[i] = malloc(width * sizeof(int));
        if (new_clusters->binary_mask[i] == NULL) {
            perror("Error allocating memory for binary_mask columns");
            for (int j = 0; j < i; j++) {
                free(new_clusters->binary_mask[j]);
            }
            free(new_clusters->binary_mask);
            free(new_clusters);
            return NULL;
        }
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
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


/* Frees one node of the list together with its mask. */
static void free_cluster(Cluster* cluster) {
    for (int i = 0; i < cluster->height; i++) {
        free(cluster->binary_mask[i]);
    }
    free(cluster->binary_mask);
    free(cluster);
}

Clusters find_clusters_attributes(Clusters clusters) {
    Cluster** link = &clusters;
    while (*link != NULL) {
        Cluster* current = *link;
        int min_x = current->width;
        int min_y = current->height;
        int max_x = -1;
        int max_y = -1;
        for (int i = 0; i < current->height; i++) {
            for (int j = 0; j < current->width; j++) {
                if (current->binary_mask[i][j] != 1) {
                    continue;
                }
                if (min_y > i) {
                    min_y = i;
                }
                if (max_y < i) {
                    max_y = i;
                }
                if (min_x > j) {
                    min_x = j;
                }
                if (max_x < j) {
                    max_x = j;
                }
            }
        }
        current->mid_x = min_x + (max_x - min_x) / 2;
        current->mid_y = min_y + (max_y - min_y) / 2;

        const int radius_x = (max_x - min_x) / 2;
        const int radius_y = (max_y - min_y) / 2;
        current->radius = (radius_x > radius_y) ? radius_x : radius_y;

        if (max_x < 0 || current->radius < MIN_BALL_RADIUS) {
            // Empty mask or blob too small to be a ball: unlink this node only.
            *link = current->next;
            free_cluster(current);
            continue;
        }

        if (current->color == ORANGE) {
            current->mid_y *= 1 - (current->radius * 0.0013);
        }
        if (current->color == YELLOW) {
            current->mid_y *= 1 + (current->radius * 0.0015);
        }
        if (current->color == ORANGE || current->color == YELLOW) {
            current->radius *= 1.12;
        }
        link = &current->next;
    }
    return clusters;
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
        free_cluster(temp);
    }
}



int dfs(int** mask, int** visited, int height, int width, int x, int y) {
    if (x < 0 || y < 0 || x >= height || y >= width || visited[x][y] || mask[x][y] == 0) {
        return 0;
    }

    visited[x][y] = 1;
    int size = 1;


    for (int d = 0; d < 4; d++) {
        size += dfs(mask, visited, height, width, x + directions[d][0], y + directions[d][1]);
    }

    return size;
}

void update_binary_mask_with_largest_cluster(Clusters clusters) {
    Clusters current = clusters;

    while (current != NULL) {
        int height = current->height;
        int width = current->width;


        int** visited = malloc(height * sizeof(int*));
        for (int i = 0; i < height; i++) {
        visited[i] = calloc(width, sizeof(int));
        }

        int largest_size = 0;
        int largest_cluster_x = -1;
        int largest_cluster_y = -1;


        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (current->binary_mask[i][j] == 1 && !visited[i][j]) {
                    int size = dfs(current->binary_mask, visited, height, width, i, j);
                    if (size > largest_size) {
                        largest_size = size;
                        largest_cluster_x = i;
                        largest_cluster_y = j;
                    }
                }
            }
        }


        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                visited[i][j] = 0;
            }
        }


        dfs(current->binary_mask, visited, height, width, largest_cluster_x, largest_cluster_y);


        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (visited[i][j]) {
                    current->binary_mask[i][j] = 1;
                } else {
                    current->binary_mask[i][j] = 0;
                }
            }
        }

        current->number_pixels = largest_size;


        for (int i = 0; i < height; i++) {
            free(visited[i]);
        }
        free(visited);
        current = current->next;
    }
}
