//
// Created by Guilhem on 25/01/2025.
//

#ifndef CLUSTER_H
#define CLUSTER_H

typedef enum {
    ORANGE,
    BLUE,
    YELLOW,
} Color;

typedef struct Cluster_ {
    int width;
    int height;
    int number_pixels;
    int** binary_mask;
    Color color;
    int mid_x;
    int mid_y;
    int radius;
    struct Cluster_* next;
} Cluster;

typedef Cluster* Clusters;


/**
 * @brief Initializes a linked list of clusters.
 *
 * @return A `Clusters` pointer initialized to NULL.
 */
Clusters init_clusters(void);

/**
 * @brief Adds a new cluster to the linked list of clusters.
 *
 * @param clusters Pointer to the head of the existing clusters linked list.
 * @param width Width of the cluster.
 * @param height Height of the cluster.
 * @param number_pixels Number of pixels in the cluster.
 * @param binary_mask 2D array representing the binary mask of the cluster.
 * @param color The color associated with the cluster (from the `Color` enum).
 * @return The updated linked list of clusters, or NULL on error.
 */
Clusters add_cluster(const Clusters clusters, const int width, const int height, const int number_pixels, int** binary_mask, const Color color);

/**
 * @brief Converts a `Color` enum value to its corresponding string representation.
 *
 * @param color The target color (defined in the `Color` enum).
 * @return A pointer to a string representing the color, or NULL on error.
 */
char* color_to_string(Color color);

int number_clusters(const Clusters clusters);

/**
 * @brief Calculates and sets the mid-point coordinates (mid_x, mid_y) and radius
 *        for each cluster in the linked list.
 *
 * @param clusters Pointer to the head of the clusters linked list.
 */
Clusters find_clusters_attributes(Clusters clusters);

/**
 * @brief Displays the information of each cluster in the linked list.
 *
 * This function iterates through the linked list of clusters and prints the
 * following information for each cluster:
 *   - Color of the cluster.
 *   - Mid-point coordinates (mid_x, mid_y).
 *   - Radius of the cluster.
 *
 * @param clusters Pointer to the head of the clusters linked list.
 */
void display_clusters(Clusters clusters);

void free_clusters(const Clusters clusters);

int dfs(int** mask, int** visited, int height, int width, int x, int y);

void update_binary_mask_with_largest_cluster(Cluster* cluster);

#endif //CLUSTER_H
