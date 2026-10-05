#!/usr/bin/env python3
"""Convert an image to the text format read by the detector, or check the dumps.

The detector does not decode image files. It reads a text dump: one header
line "width height 3", then the red plane, the green plane and the blue plane,
each as `height` lines of `width` integers between 0 and 255.

    python scripts/image_to_text.py photo.jpg photo.txt        # convert one image
    python scripts/image_to_text.py photo.jpg photo.txt --size 300
    python scripts/image_to_text.py --verify                    # compare IMG_300/*.txt with the JPEG files

Requires Pillow (see requirements.txt).
"""

import argparse
import sys
from pathlib import Path

from PIL import Image

from common import PHOTO_DIR, image_names

# JPEG decoders may round differently by one or two levels; a larger gap means a damaged dump.
VERIFY_TOLERANCE = 2


def planes(image):
    """Red, green and blue planes of an image, each as a list of rows of integers."""
    image = image.convert("RGB")
    width, height = image.size
    result = []
    for channel in image.split():
        values = channel.tobytes()
        result.append([list(values[row * width:(row + 1) * width]) for row in range(height)])
    return width, height, result


def to_text(image):
    width, height, channels = planes(image)
    lines = [f"{width} {height} 3"]
    for channel in channels:
        lines += [" ".join(str(value) for value in row) for row in channel]
    return "\n".join(lines) + "\n"


def verify():
    """Compare every committed dump with the pixels decoded from its JPEG file."""
    worst = 0
    damaged = 0
    for name in image_names():
        width, height, channels = planes(Image.open(PHOTO_DIR / f"{name}.jpeg"))
        expected = [value for channel in channels for row in channel for value in row]
        tokens = (PHOTO_DIR / f"{name}.txt").read_text().split()
        if tokens[:3] != [str(width), str(height), "3"] or len(tokens) != 3 + len(expected):
            print(f"{name}: header or sample count does not match the {width} x {height} photo")
            damaged += 1
            continue
        difference = max(abs(int(token) - value) for token, value in zip(tokens[3:], expected))
        worst = max(worst, difference)
        if difference > VERIFY_TOLERANCE:
            print(f"{name}: samples differ from the photo by up to {difference}")
            damaged += 1
    print(f"{len(image_names())} dumps compared with their photo: largest difference {worst}, {damaged} damaged")
    return 1 if damaged else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("source", nargs="?", type=Path, help="image file readable by Pillow")
    parser.add_argument("target", nargs="?", type=Path, help="text file to write")
    parser.add_argument("--size", type=int, help="resize to SIZE x SIZE first (the detector was tuned on 300 x 300)")
    parser.add_argument("--verify", action="store_true", help="check the dumps of IMG_300/ against the JPEG files")
    args = parser.parse_args()

    if args.verify:
        return verify()
    if args.source is None or args.target is None:
        parser.error("give a source image and a target text file, or --verify")

    image = Image.open(args.source)
    if args.size:
        image = image.convert("RGB").resize((args.size, args.size), Image.LANCZOS)
    args.target.write_text(to_text(image), newline="\n")
    print(f"wrote {args.target} ({image.size[0]} x {image.size[1]})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
