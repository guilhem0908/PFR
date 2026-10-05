//
// Created by Guilhem on 25/01/2025.
//

#include <stdio.h>
#include "cluster.h"

#include <stdlib.h>

// 4-connected neighbourhood, as (row, column) offsets.
static const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

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

const char* color_to_string(const Color color) {
    switch (color) {
        case ORANGE: return "orange";
        case BLUE: return "blue";
        case YELLOW: return "yellow";
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

        // Empirical corrections from the integration version, tuned on the 300 x 300
        // test photos: the thresholds do not cover the whole surface of orange and
        // yellow balls, so the bounding box of the mask is off-centre and too small.
        // They are written with integers (truncated like the original floating-point
        // expressions) so that the output does not depend on floating-point rounding.
        if (current->color == ORANGE) {
            // mid_y * (1 - 0.0013 * radius)
            current->mid_y = current->mid_y * (10000 - 13 * current->radius) / 10000;
        }
        if (current->color == YELLOW) {
            // mid_y * (1 + 0.0015 * radius)
            current->mid_y = current->mid_y * (10000 + 15 * current->radius) / 10000;
        }
        if (current->color == ORANGE || current->color == YELLOW) {
            // radius * 1.12
            current->radius = current->radius * 112 / 100;
        }
        link = &current->next;
    }
    return clusters;
}


void display_clusters(const Clusters clusters) {
    if (clusters == NULL) {
        printf("No ball detected.\n");
        return;
    }

    Clusters current = clusters;
    while (current != NULL) {
        printf("%s ball detected at (%d, %d), radius %d.\n", color_to_string(current->color),
               current->mid_x, current->mid_y, current->radius);
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

/*
 * Labels the 4-connected component that contains (start_row, start_col) and
 * returns its size in pixels. The traversal uses an explicit stack of pixel
 * indices: every pixel is pushed at most once (it is labelled when pushed),
 * so a stack of width * height entries can never overflow.
 */
static int flood_fill(int** mask, int* labels, int* stack, const int height, const int width,
                      const int start_row, const int start_col, const int label) {
    int stack_size = 0;
    int size = 0;

    labels[start_row * width + start_col] = label;
    stack[stack_size++] = start_row * width + start_col;

    while (stack_size > 0) {
        const int index = stack[--stack_size];
        const int row = index / width;
        const int col = index % width;
        size++;

        for (int d = 0; d < 4; d++) {
            const int next_row = row + directions[d][0];
            const int next_col = col + directions[d][1];
            if (next_row < 0 || next_col < 0 || next_row >= height || next_col >= width) {
                continue;
            }
            const int next_index = next_row * width + next_col;
            if (mask[next_row][next_col] != 1 || labels[next_index] != 0) {
                continue;
            }
            labels[next_index] = label;
            stack[stack_size++] = next_index;
        }
    }

    return size;
}

bool update_binary_mask_with_largest_cluster(const Clusters clusters) {
    Clusters current = clusters;

    while (current != NULL) {
        const int height = current->height;
        const int width = current->width;

        int* labels = calloc((size_t) height * width, sizeof(int));
        int* stack = malloc((size_t) height * width * sizeof(int));
        if (!labels || !stack) {
            perror("Error allocating memory for the connected-component search");
            free(labels);
            free(stack);
            return false;
        }

        // Label every component in raster order; on equal sizes the first one found is kept.
        int number_labels = 0;
        int largest_label = 0;
        int largest_size = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (current->binary_mask[i][j] == 1 && labels[i * width + j] == 0) {
                    number_labels++;
                    const int size = flood_fill(current->binary_mask, labels, stack, height, width, i, j, number_labels);
                    if (size > largest_size) {
                        largest_size = size;
                        largest_label = number_labels;
                    }
                }
            }
        }

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                current->binary_mask[i][j] = (largest_label != 0 && labels[i * width + j] == largest_label) ? 1 : 0;
            }
        }
        current->number_pixels = largest_size;

        free(labels);
        free(stack);
        current = current->next;
    }

    return true;
}
