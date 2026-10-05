/*
 * Unit tests for the image and cluster modules. No test image is read from
 * disk: every input is built in memory.
 *
 * Failed checks are printed on stdout; the messages the library prints on
 * stderr for the invalid inputs tested here are expected.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cluster.h"
#include "image_process.h"

static int checks = 0;
static int failures = 0;

#define CHECK(condition) \
    do { \
        checks++; \
        if (!(condition)) { \
            failures++; \
            printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
        } \
    } while (0)

static int** new_mask(const int width, const int height) {
    int** mask = malloc(height * sizeof(int*));
    for (int i = 0; i < height; i++) {
        mask[i] = calloc(width, sizeof(int));
    }
    return mask;
}

static void free_mask(int** mask, const int height) {
    for (int i = 0; i < height; i++) {
        free(mask[i]);
    }
    free(mask);
}

/* Sets the rectangle [left, right] x [top, bottom] (inclusive bounds) to 1. */
static void fill_rectangle(int** mask, const int left, const int top, const int right, const int bottom) {
    for (int i = top; i <= bottom; i++) {
        for (int j = left; j <= right; j++) {
            mask[i][j] = 1;
        }
    }
}

static void paint(const ImageData image, const int row, const int col, const int R, const int G, const int B) {
    image->red_components[row][col] = R;
    image->green_components[row][col] = G;
    image->blue_components[row][col] = B;
}

static void test_quantize_pixel(void) {
    CHECK(quantize_pixel(255, 0, 255, 1) == 5);        /* 1 0 1 */
    CHECK(quantize_pixel(255, 128, 0, 2) == 56);       /* 11 10 00 */
    CHECK(quantize_pixel(127, 127, 127, 1) == 0);
    CHECK(quantize_pixel(128, 128, 128, 1) == 7);
    CHECK(quantize_pixel(1, 2, 3, 8) == 66051);        /* 0x010203 */
    CHECK(quantize_pixel(255, 255, 255, 3) == 511);    /* 2^9 - 1 */
    CHECK(quantize_pixel(0, 0, 0, 0) == -1);
    CHECK(quantize_pixel(0, 0, 0, 9) == -1);
}

static void test_parse_image_text(void) {
    const ImageData image = parse_image_text("3 2 3\n1 2 3\n4 5 6\n10 20 30\n40 50 60\n100 110 120\r\n130 140 255\r\n");
    CHECK(image != NULL);
    if (image != NULL) {
        CHECK(image->width == 3);
        CHECK(image->height == 2);
        CHECK(image->red_components[0][0] == 1);
        CHECK(image->red_components[1][2] == 6);
        CHECK(image->green_components[0][1] == 20);
        CHECK(image->green_components[1][0] == 40);
        CHECK(image->blue_components[0][2] == 120);
        CHECK(image->blue_components[1][2] == 255);
        CHECK(image->quantized_pixels == NULL);
    }
    free_image_data(image);

    CHECK(parse_image_text("") == NULL);                           /* no header */
    CHECK(parse_image_text("2 2") == NULL);                        /* incomplete header */
    CHECK(parse_image_text("1 1 1 7") == NULL);                    /* greyscale */
    CHECK(parse_image_text("0 4 3") == NULL);                      /* empty image */
    CHECK(parse_image_text("-2 2 3") == NULL);                     /* negative side */
    CHECK(parse_image_text("5000 2 3") == NULL);                   /* side above the limit */
    CHECK(parse_image_text("2 1 3 1 2 3 4 5") == NULL);            /* one sample missing */
    CHECK(parse_image_text("1 1 3 0 0 256") == NULL);              /* sample above 255 */
    CHECK(parse_image_text("1 1 3 0 -1 0") == NULL);               /* negative sample */
    CHECK(parse_image_text("1 1 3 12 abc 3") == NULL);             /* not a number */
    CHECK(parse_image_text("1 1 3 99999999999999999999 0 0") == NULL);
}

static void test_quantize_image(void) {
    /* Wider than tall, so a row sized with the height would be too short. */
    const ImageData image = create_image_data(5, 2);
    CHECK(image != NULL);
    if (image == NULL) {
        return;
    }
    paint(image, 0, 0, 255, 128, 0);
    paint(image, 1, 4, 64, 64, 192);

    CHECK(!quantize_image(image, 0));
    CHECK(!quantize_image(image, 9));
    CHECK(image->quantized_pixels == NULL);

    CHECK(quantize_image(image, 2));
    CHECK(image->n == 2);
    CHECK(image->quantized_pixels != NULL);
    if (image->quantized_pixels != NULL) {
        CHECK(image->quantized_pixels[0][0] == 56);    /* 11 10 00 */
        CHECK(image->quantized_pixels[1][4] == 23);    /* 01 01 11 */
        CHECK(image->quantized_pixels[0][1] == 0);
    }

    CHECK(quantize_image(image, 1));
    CHECK(image->n == 1);
    CHECK(image->quantized_pixels[0][0] == 6);         /* 1 1 0 */
    free_image_data(image);

    CHECK(create_image_data(0, 10) == NULL);
    CHECK(create_image_data(10, 5000) == NULL);
}

static void test_find_clusters(void) {
    const ImageData image = create_image_data(6, 4);
    CHECK(image != NULL);
    if (image == NULL) {
        return;
    }
    for (int i = 0; i < image->height; i++) {
        for (int j = 0; j < image->width; j++) {
            paint(image, i, j, 128, 128, 128);
        }
    }

    Clusters clusters = NULL;
    CHECK(find_clusters(image, &clusters));
    CHECK(clusters == NULL);                           /* grey matches no colour */

    int thresholds[6];
    get_thresholds(ORANGE, thresholds);
    paint(image, 0, 0, thresholds[0], thresholds[2], thresholds[4]);          /* lower bounds: inside */
    paint(image, 0, 1, thresholds[1], thresholds[3], thresholds[5]);          /* upper bounds: inside */
    paint(image, 0, 2, thresholds[0] - 1, thresholds[2], thresholds[4]);      /* just outside */
    paint(image, 0, 3, thresholds[1], thresholds[3] + 1, thresholds[5]);      /* just outside */
    get_thresholds(YELLOW, thresholds);
    paint(image, 3, 5, thresholds[0], thresholds[2], thresholds[4]);

    CHECK(find_clusters(image, &clusters));
    CHECK(number_clusters(clusters) == 2);
    if (number_clusters(clusters) == 2) {
        /* Clusters are prepended, so the last colour searched comes first. */
        CHECK(clusters->color == YELLOW);
        CHECK(clusters->number_pixels == 1);
        CHECK(clusters->binary_mask[3][5] == 1);
        CHECK(clusters->binary_mask[0][0] == 0);
        CHECK(clusters->width == 6 && clusters->height == 4);

        CHECK(clusters->next->color == ORANGE);
        CHECK(clusters->next->number_pixels == 2);
        CHECK(clusters->next->binary_mask[0][0] == 1);
        CHECK(clusters->next->binary_mask[0][1] == 1);
        CHECK(clusters->next->binary_mask[0][2] == 0);
        CHECK(clusters->next->binary_mask[0][3] == 0);
        CHECK(clusters->next->binary_mask[3][5] == 0);
    }
    free_clusters(clusters);
    free_image_data(image);
}

static void test_add_cluster_copies_the_mask(void) {
    int** mask = new_mask(4, 3);
    mask[1][2] = 1;

    Clusters clusters = add_cluster(init_clusters(), 4, 3, 1, mask, BLUE);
    CHECK(clusters != NULL);
    mask[1][2] = 0;
    mask[0][0] = 1;
    if (clusters != NULL) {
        CHECK(clusters->binary_mask[1][2] == 1);
        CHECK(clusters->binary_mask[0][0] == 0);
        CHECK(clusters->mid_x == -1 && clusters->mid_y == -1 && clusters->radius == -1);
        CHECK(clusters->next == NULL);
    }
    CHECK(number_clusters(clusters) == 1);
    CHECK(number_clusters(NULL) == 0);

    free_clusters(clusters);
    free_clusters(NULL);
    free_mask(mask, 3);
}

static void test_largest_component(void) {
    /*
     * . . . . . . . .
     * . A A . . . B B      A: 3 pixels, B: 5 pixels, C: 1 pixel that touches
     * . A . . . B B B         A only by a corner (4-connectivity keeps it apart).
     * C . . . . . . .
     */
    int** mask = new_mask(8, 4);
    mask[1][1] = mask[1][2] = mask[2][1] = 1;
    mask[1][6] = mask[1][7] = mask[2][5] = mask[2][6] = mask[2][7] = 1;
    mask[3][0] = 1;

    Clusters clusters = add_cluster(init_clusters(), 8, 4, 9, mask, BLUE);
    CHECK(update_binary_mask_with_largest_cluster(clusters));
    CHECK(clusters->number_pixels == 5);
    int remaining = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            remaining += clusters->binary_mask[i][j];
        }
    }
    CHECK(remaining == 5);
    CHECK(clusters->binary_mask[1][6] == 1 && clusters->binary_mask[2][5] == 1);
    CHECK(clusters->binary_mask[1][1] == 0 && clusters->binary_mask[3][0] == 0);
    free_clusters(clusters);
    free_mask(mask, 4);

    /* Two components of the same size: the first one in raster order is kept. */
    mask = new_mask(7, 2);
    mask[0][5] = mask[1][5] = 1;
    mask[1][0] = mask[1][1] = 1;
    clusters = add_cluster(init_clusters(), 7, 2, 4, mask, ORANGE);
    CHECK(update_binary_mask_with_largest_cluster(clusters));
    CHECK(clusters->number_pixels == 2);
    CHECK(clusters->binary_mask[0][5] == 1 && clusters->binary_mask[1][5] == 1);
    CHECK(clusters->binary_mask[1][0] == 0 && clusters->binary_mask[1][1] == 0);
    free_clusters(clusters);
    free_mask(mask, 2);

    CHECK(update_binary_mask_with_largest_cluster(NULL));
}

static void test_cluster_attributes(void) {
    const int width = 120;
    const int height = 60;

    /* Columns 10-40 and rows 20-50: centre (25, 35), half-sides 15. */
    int** large = new_mask(width, height);
    fill_rectangle(large, 10, 20, 40, 50);
    /* Columns 60-83 and rows 5-25: half-sides 11 and 10, below MIN_BALL_RADIUS. */
    int** small = new_mask(width, height);
    fill_rectangle(small, 60, 5, 83, 25);
    /* Columns 50-110 and rows 30-56: wider than tall, radius from the width. */
    int** wide = new_mask(width, height);
    fill_rectangle(wide, 50, 30, 110, 56);

    /* A small cluster between two kept ones must be removed without touching its neighbours. */
    Clusters clusters = init_clusters();
    clusters = add_cluster(clusters, width, height, 31 * 31, large, ORANGE);
    clusters = add_cluster(clusters, width, height, 24 * 21, small, BLUE);
    clusters = add_cluster(clusters, width, height, 31 * 31, large, YELLOW);
    clusters = find_clusters_attributes(clusters);
    CHECK(number_clusters(clusters) == 2);
    if (number_clusters(clusters) == 2) {
        CHECK(clusters->color == YELLOW);
        CHECK(clusters->mid_x == 25);
        CHECK(clusters->mid_y == 35);                  /* 35 * (1 + 0.0015 * 15) = 35.79 */
        CHECK(clusters->radius == 16);                 /* 15 * 1.12 = 16.8 */
        CHECK(clusters->next->color == ORANGE);
        CHECK(clusters->next->mid_x == 25);
        CHECK(clusters->next->mid_y == 34);            /* 35 * (1 - 0.0013 * 15) = 34.32 */
        CHECK(clusters->next->radius == 16);
    }
    free_clusters(clusters);

    /* Blue balls get no correction; the radius is the larger half-side of the box. */
    clusters = add_cluster(init_clusters(), width, height, 61 * 27, wide, BLUE);
    clusters = find_clusters_attributes(clusters);
    CHECK(number_clusters(clusters) == 1);
    if (clusters != NULL) {
        CHECK(clusters->mid_x == 80);
        CHECK(clusters->mid_y == 43);
        CHECK(clusters->radius == 30);
    }
    free_clusters(clusters);

    /* Small clusters at the head, or alone, leave an empty list. */
    clusters = init_clusters();
    clusters = add_cluster(clusters, width, height, 24 * 21, small, ORANGE);
    clusters = add_cluster(clusters, width, height, 24 * 21, small, BLUE);
    clusters = add_cluster(clusters, width, height, 24 * 21, small, YELLOW);
    clusters = find_clusters_attributes(clusters);
    CHECK(clusters == NULL);
    CHECK(find_clusters_attributes(NULL) == NULL);

    /* The yellow correction lands exactly on a whole number: 50 * 1.06 = 53. */
    int** edge = new_mask(300, 300);
    fill_rectangle(edge, 110, 10, 190, 90);
    clusters = add_cluster(init_clusters(), 300, 300, 81 * 81, edge, YELLOW);
    clusters = find_clusters_attributes(clusters);
    CHECK(clusters != NULL);
    if (clusters != NULL) {
        CHECK(clusters->mid_x == 150);
        CHECK(clusters->mid_y == 53);
        CHECK(clusters->radius == 44);                 /* 40 * 1.12 = 44.8 */
    }
    free_clusters(clusters);

    free_mask(large, height);
    free_mask(small, height);
    free_mask(wide, height);
    free_mask(edge, 300);
}

static void test_color_to_string(void) {
    CHECK(color_to_string(ORANGE)[0] == 'o');
    CHECK(color_to_string(BLUE)[0] == 'b');
    CHECK(color_to_string(YELLOW)[0] == 'y');
    CHECK(color_to_string((Color) 42) == NULL);
}

int main(void) {
    test_quantize_pixel();
    test_parse_image_text();
    test_quantize_image();
    test_find_clusters();
    test_add_cluster_copies_the_mask();
    test_largest_component();
    test_cluster_attributes();
    test_color_to_string();

    printf("unit tests: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
