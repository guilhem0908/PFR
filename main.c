#include "image_process.h"
#include "file_operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_RESULT_PATH "result.txt"

static void print_usage(FILE* stream, const char* program) {
    fprintf(stream,
            "Usage: %s <image.txt> [-o <result.txt>] [--masks <prefix>]\n"
            "\n"
            "Detects orange, blue and yellow balls in a text image and writes the number\n"
            "of balls found, followed by one line 'colour x y radius' per ball.\n"
            "\n"
            "Options:\n"
            "  -o, --output <file>  result file (default: " DEFAULT_RESULT_PATH ")\n"
            "  --masks <prefix>     also write the binary masks as PGM images:\n"
            "                       <prefix><colour>_threshold.pgm (pixels inside the\n"
            "                       colour thresholds) and <prefix><colour>_largest.pgm\n"
            "                       (largest connected component)\n"
            "  -h, --help           show this help\n",
            program);
}

/* Writes the mask of every cluster of the list to '<prefix><colour>_<stage>.pgm'. */
static bool write_masks(const Clusters clusters, const char* prefix, const char* stage) {
    for (Clusters current = clusters; current != NULL; current = current->next) {
        char path[1024];
        const int length = snprintf(path, sizeof(path), "%s%s_%s.pgm", prefix, color_to_string(current->color), stage);
        if (length < 0 || (size_t) length >= sizeof(path)) {
            fprintf(stderr, "The mask prefix '%s' is too long.\n", prefix);
            return false;
        }
        if (!write_mask_pgm(path, current->binary_mask, current->width, current->height)) {
            fprintf(stderr, "Could not write the mask '%s'.\n", path);
            return false;
        }
    }
    return true;
}

/* Writes the number of clusters, then one line 'colour mid_x mid_y radius' per cluster. */
static bool write_result(const char* path, const Clusters clusters) {
    FILE* result_file = fopen(path, "w");
    if (!result_file) {
        perror("Error opening the result file");
        return false;
    }

    fprintf(result_file, "%d\n", number_clusters(clusters));
    for (Clusters current_cluster = clusters; current_cluster != NULL; current_cluster = current_cluster->next) {
        fprintf(result_file, "%s %d %d %d\n", color_to_string(current_cluster->color),
                current_cluster->mid_x, current_cluster->mid_y, current_cluster->radius);
    }

    const bool written = !ferror(result_file);
    return fclose(result_file) == 0 && written;
}

int main(int argc, char *argv[]) {
    const char* program = argc > 0 ? argv[0] : "PFR";
    const char* image_path = NULL;
    const char* result_path = DEFAULT_RESULT_PATH;
    const char* masks_prefix = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(stdout, program);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Option '%s' needs a file name.\n", argv[i]);
                return 2;
            }
            result_path = argv[++i];
        } else if (strcmp(argv[i], "--masks") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Option '--masks' needs a prefix.\n");
                return 2;
            }
            masks_prefix = argv[++i];
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "Unknown option '%s'.\n", argv[i]);
            print_usage(stderr, program);
            return 2;
        } else if (image_path == NULL) {
            image_path = argv[i];
        } else {
            fprintf(stderr, "Only one image can be processed at a time.\n");
            print_usage(stderr, program);
            return 2;
        }
    }
    if (image_path == NULL) {
        print_usage(stderr, program);
        return 2;
    }

    const ImageData image_data = extract_image_text_data(image_path);
    if (!image_data) {
        fprintf(stderr, "Could not load the image '%s'.\n", image_path);
        return EXIT_FAILURE;
    }

    int status = EXIT_FAILURE;
    Clusters clusters = NULL;
    if (find_clusters(image_data, &clusters) &&
        (masks_prefix == NULL || write_masks(clusters, masks_prefix, "threshold")) &&
        update_binary_mask_with_largest_cluster(clusters) &&
        (masks_prefix == NULL || write_masks(clusters, masks_prefix, "largest"))) {
        clusters = find_clusters_attributes(clusters);
        display_clusters(clusters);
        if (write_result(result_path, clusters)) {
            status = EXIT_SUCCESS;
        }
    }

    free_clusters(clusters);
    free_image_data(image_data);

    return status;
}
