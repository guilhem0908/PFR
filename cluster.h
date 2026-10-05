//
// Created by Guilhem on 25/01/2025.
//

#ifndef CLUSTER_H
#define CLUSTER_H

#include <stdbool.h>

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

/** Blobs whose radius is below this value (in pixels) are not reported as balls. */
#define MIN_BALL_RADIUS 13


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

/**
 * @brief Counts the clusters of the linked list.
 *
 * @param clusters Pointer to the head of the clusters linked list.
 * @return The number of clusters (0 for an empty list).
 */
int number_clusters(const Clusters clusters);

/**
 * @brief Calculates and sets the mid-point coordinates (mid_x, mid_y) and radius
 *        for each cluster in the linked list, from the bounding box of its mask.
 *
 * Clusters whose radius is below MIN_BALL_RADIUS are removed from the list and freed.
 * For orange and yellow balls the row of the centre and the radius then receive
 * the empirical corrections tuned on the test photos.
 *
 * @param clusters Pointer to the head of the clusters linked list.
 * @return The head of the list once the small clusters are removed (may be NULL).
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

/**
 * @brief Frees every cluster of the list together with its binary mask.
 *
 * @param clusters Pointer to the head of the clusters linked list (may be NULL).
 */
void free_clusters(const Clusters clusters);

/**
 * @brief Keeps only the largest 4-connected component in the binary mask of
 *        each cluster and updates its pixel count.
 *
 * The components are found with an iterative flood fill (explicit stack), so
 * the depth of the search does not depend on the size of the component.
 *
 * @param clusters Pointer to the head of the clusters linked list.
 * @return true on success, false if memory cannot be allocated.
 */
bool update_binary_mask_with_largest_cluster(Clusters clusters);

#endif //CLUSTER_H
