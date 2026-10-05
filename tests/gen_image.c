/*
 * Test helper: writes a synthetic text image to stdout.
 *
 * Usage: gen_image <width> <height> [<cx> <cy> <radius> <R> <G> <B>]...
 *
 * The background is mid grey (128, 128, 128), which no colour class accepts.
 * Each group of six arguments paints a filled disc; later discs are drawn
 * over earlier ones. The output uses the detector's input format: a header
 * line "width height 3" followed by the red, green and blue planes.
 */

#include <stdio.h>
#include <stdlib.h>

#define BACKGROUND 128

int main(int argc, char* argv[]) {
    if (argc < 3 || (argc - 3) % 6 != 0) {
        fprintf(stderr, "Usage: %s <width> <height> [<cx> <cy> <radius> <R> <G> <B>]...\n", argv[0]);
        return 2;
    }

    const int width = atoi(argv[1]);
    const int height = atoi(argv[2]);
    const int discs = (argc - 3) / 6;
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "gen_image: width and height must be positive.\n");
        return 2;
    }

    printf("%d %d 3\n", width, height);
    for (int channel = 0; channel < 3; channel++) {
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                int value = BACKGROUND;
                for (int d = 0; d < discs; d++) {
                    char** disc = argv + 3 + 6 * d;
                    const int dx = col - atoi(disc[0]);
                    const int dy = row - atoi(disc[1]);
                    const int radius = atoi(disc[2]);
                    if (dx * dx + dy * dy <= radius * radius) {
                        value = atoi(disc[3 + channel]);
                    }
                }
                printf(col + 1 < width ? "%d " : "%d\n", value);
            }
        }
    }
    return 0;
}
