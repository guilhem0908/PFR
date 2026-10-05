#!/usr/bin/env python3
"""Draw the figures of docs/ from the output of the compiled detector.

    docs/contact_sheet.png   the 20 photos with the detected circles
    docs/labels.png          the same sheet with the reference labels added (dashed)
    docs/pipeline.png        photo / colour masks / result for two scenes

Python only draws here: circles are the centres and radii written by the C
program, masks are the PGM images it exports with --masks.

    python scripts/make_figures.py

Requires Pillow (see requirements.txt).
"""

import argparse
import functools
import sys

from PIL import Image, ImageDraw, ImageFont

from common import (DOCS_DIR, PHOTO_DIR, WORK_DIR, find_binary, image_names, load_ground_truth,
                    mask_bounding_box, read_pgm, run_detector)
from evaluate import count, score_image

SUPERSAMPLE = 4          # overlays are drawn 4x larger, then reduced, to smooth the circles
TILE = 300               # the photos are 300 x 300 and shown at their native size on the sheets
PANEL_SCALE = 2          # magnification of the pipeline panels

BACKGROUND = (14, 17, 22)
PANEL_EDGE = (48, 54, 61)
TEXT = (230, 237, 243)
MUTED = (139, 148, 158)
OUTLINE = (0, 0, 0, 170)
LABEL = (255, 255, 255, 235)

RING = {"orange": (255, 138, 36), "blue": (64, 160, 255), "yellow": (255, 224, 64)}
MASK_BRIGHT = {"orange": (255, 138, 36), "blue": (64, 160, 255), "yellow": (255, 224, 64)}
MASK_DIM = {"orange": (110, 52, 8), "blue": (16, 58, 112), "yellow": (104, 90, 14)}


@functools.lru_cache(maxsize=None)
def font(size):
    return ImageFont.load_default(size=size)


def load_photo(name):
    return Image.open(PHOTO_DIR / f"{name}.jpeg").convert("RGB")


class Overlay:
    """Transparent layer drawn in image coordinates and supersampled for smooth edges.

    `scale` is the magnification of the image in the final figure. Positions and
    radii are given in image pixels, stroke widths in pixels of the final figure.
    """

    def __init__(self, width, height, scale=1):
        self.final_size = (round(width * scale), round(height * scale))
        self.unit = scale * SUPERSAMPLE
        self.image = Image.new("RGBA", (self.final_size[0] * SUPERSAMPLE, self.final_size[1] * SUPERSAMPLE))
        self.draw = ImageDraw.Draw(self.image)

    def _circle(self, x, y, radius):
        """Bounding box, in layer pixels, of a circle whose radius is already in layer pixels."""
        return [x * self.unit - radius, y * self.unit - radius, x * self.unit + radius, y * self.unit + radius]

    def ring(self, x, y, radius, colour, width=2.6):
        """Circle of the given colour, centred on the radius, with a dark edge on both sides."""
        stroke = width * SUPERSAMPLE
        edge = 1.1 * SUPERSAMPLE
        middle = radius * self.unit
        self.draw.ellipse(self._circle(x, y, middle + stroke / 2 + edge), outline=OUTLINE,
                          width=round(stroke + 2 * edge))
        self.draw.ellipse(self._circle(x, y, middle + stroke / 2), outline=colour + (255,), width=round(stroke))

    def dashed_circle(self, x, y, radius, colour=LABEL, dashes=30, width=1.4):
        stroke = width * SUPERSAMPLE
        step = 360 / dashes
        for index in range(dashes):
            self.draw.arc(self._circle(x, y, radius * self.unit + stroke / 2), index * step,
                          index * step + step * 0.55, fill=colour, width=round(stroke))

    def cross(self, x, y, colour, arm=4.5, width=1.8):
        cx, cy = x * self.unit, y * self.unit
        arm *= SUPERSAMPLE
        for fill, stroke in ((OUTLINE, width + 2.2), (colour + (255,), width)):
            self.draw.line([cx - arm, cy, cx + arm, cy], fill=fill, width=round(stroke * SUPERSAMPLE))
            self.draw.line([cx, cy - arm, cx, cy + arm], fill=fill, width=round(stroke * SUPERSAMPLE))

    def rectangle(self, left, top, right, bottom, colour, width=1.2):
        self.draw.rectangle([left * self.unit, top * self.unit, right * self.unit, bottom * self.unit],
                            outline=colour, width=round(width * SUPERSAMPLE))

    def composite_onto(self, image, origin):
        """Reduce the layer to its final size and blend it over `image` at `origin`."""
        layer = self.image.convert("RGBa").resize(self.final_size, Image.LANCZOS).convert("RGBA")
        image.paste(layer, origin, layer)


def draw_detections(overlay, detections, labels=None):
    """Detected circles (pixel centres sit at +0.5) and, optionally, the reference labels."""
    for ball in labels or []:
        overlay.dashed_circle(ball["x"] + 0.5, ball["y"] + 0.5, ball["radius"])
    for ball in detections:
        colour = RING[ball["colour"]]
        overlay.ring(ball["x"] + 0.5, ball["y"] + 0.5, ball["radius"], colour)
        overlay.cross(ball["x"] + 0.5, ball["y"] + 0.5, colour)


def chip(draw, x, y, text, size=13, fill=(14, 17, 22, 205), colour=TEXT, anchor="la"):
    """Text on a rounded dark background; returns the width used."""
    face = font(size)
    left, top, right, bottom = draw.textbbox((0, 0), text, font=face)
    width, height = right - left + 12, size + 9
    if anchor == "ra":
        x -= width
    draw.rounded_rectangle([x, y, x + width, y + height], radius=5, fill=fill)
    draw.text((x + 6, y + 3), text, font=face, fill=colour)
    return width


def contact_sheet(path, detections, labels, with_labels):
    """4 rows of 5 photos with the detected circles and a one-line summary."""
    names = image_names()
    columns, gap, margin, header, footer = 5, 10, 22, 74, 40
    rows = (len(names) + columns - 1) // columns
    width = 2 * margin + columns * TILE + (columns - 1) * gap
    height = header + rows * TILE + (rows - 1) * gap + footer
    sheet = Image.new("RGB", (width, height), BACKGROUND)
    draw = ImageDraw.Draw(sheet, "RGBA")

    counts = count({name: score_image(detections[name], labels[name]) for name in names})
    draw.text((margin, 16), "Colour-ball detector in C: the 20 test photos", font=font(25), fill=TEXT)
    summary = (f"{counts['balls_found']} / {counts['balls']} balls found with the right colour"
               f"   |   {counts['false_detections']} false detections"
               f"   |   {counts['empty_scenes_without_detection']} / {counts['empty_scenes']} empty scenes left empty")
    draw.text((margin, 47), summary, font=font(15), fill=MUTED)

    for index, name in enumerate(names):
        left = margin + (index % columns) * (TILE + gap)
        top = header + (index // columns) * (TILE + gap)
        sheet.paste(load_photo(name), (left, top))
        overlay = Overlay(TILE, TILE)
        draw_detections(overlay, detections[name], labels[name] if with_labels else None)
        overlay.composite_onto(sheet, (left, top))
        draw.rectangle([left - 1, top - 1, left + TILE, top + TILE], outline=PANEL_EDGE)
        used = chip(draw, left + 7, top + TILE - 29, name.replace("_", " "))
        if not labels[name]:
            verdict = "empty scene, nothing detected" if not detections[name] else "empty scene"
            chip(draw, left + 7 + used + 6, top + TILE - 29, verdict, colour=MUTED)

    y = height - footer + 12
    x = margin
    for colour in ("orange", "blue", "yellow"):
        draw.ellipse([x, y + 3, x + 12, y + 15], outline=RING[colour] + (255,), width=3)
        draw.text((x + 19, y), colour, font=font(14), fill=TEXT)
        x += 30 + draw.textlength(colour, font=font(14)) + 14
    legend = "ring and cross: centre and radius written by the C program"
    if with_labels:
        legend += "   |   dashed white: reference label (IMG_300/ground_truth.csv)"
    draw.text((x + 8, y), legend, font=font(14), fill=MUTED)

    sheet.save(path, optimize=True)


def mask_panel(masks, detections, size):
    """Colour masks of one image.

    Pixels inside the thresholds of a colour are dim, its largest component is
    bright, and the bounding box is drawn for the components reported as balls.
    """
    panel = Image.new("RGB", (TILE, TILE), (6, 8, 11))
    pixels = panel.load()
    boxes = []
    for colour in ("orange", "blue", "yellow"):
        if f"{colour}_threshold" not in masks:
            continue
        _, _, threshold = read_pgm(masks[f"{colour}_threshold"])
        _, _, largest = read_pgm(masks[f"{colour}_largest"])
        for y in range(TILE):
            for x in range(TILE):
                if largest[y][x]:
                    pixels[x, y] = MASK_BRIGHT[colour]
                elif threshold[y][x]:
                    pixels[x, y] = MASK_DIM[colour]
        if any(ball["colour"] == colour for ball in detections):
            boxes.append(mask_bounding_box(largest))
    panel = panel.resize((size, size), Image.NEAREST)
    overlay = Overlay(TILE, TILE, scale=size / TILE)
    for min_x, min_y, max_x, max_y in boxes:
        overlay.rectangle(min_x - 1, min_y - 1, max_x + 2, max_y + 2, (255, 255, 255, 170))
    overlay.composite_onto(panel, (0, 0))
    return panel


def overlaps(x, y, width, ball, height=24):
    """True when a label box (panel pixels) touches the bounding square of a detected ball."""
    cx, cy, radius = ((ball[key] + 0.5) * PANEL_SCALE for key in ("x", "y", "radius"))
    return x < cx + radius and x + width > cx - radius and y < cy + radius and y + height > cy - radius


def pipeline_figure(path, binary, names):
    """One row per scene: input photo, colour masks, detected balls."""
    size = TILE * PANEL_SCALE
    gap, margin, header, row_gap, footer = 16, 22, 92, 18, 62
    width = 2 * margin + 3 * size + 2 * gap
    height = header + len(names) * size + (len(names) - 1) * row_gap + footer
    figure = Image.new("RGB", (width, height), BACKGROUND)
    draw = ImageDraw.Draw(figure, "RGBA")

    draw.text((margin, 14), "From a text image to a ball position", font=font(27), fill=TEXT)
    titles = ("1. Input: 300 x 300 RGB image", "2. Colour thresholds, largest component", "3. Output: centre and radius")
    for column, title in enumerate(titles):
        draw.text((margin + column * (size + gap), 58), title, font=font(18), fill=MUTED)

    for row, name in enumerate(names):
        top = header + row * (size + row_gap)
        detections, masks = run_detector(binary, PHOTO_DIR / f"{name}.txt", WORK_DIR / "figures", masks=True)
        photo = load_photo(name).resize((size, size), Image.LANCZOS)

        figure.paste(photo, (margin, top))
        chip(draw, margin + 10, top + 10, name.replace("_", " "), size=15)

        figure.paste(mask_panel(masks, detections, size), (margin + size + gap, top))

        left = margin + 2 * (size + gap)
        figure.paste(photo, (left, top))
        overlay = Overlay(TILE, TILE, scale=PANEL_SCALE)
        draw_detections(overlay, detections)
        overlay.composite_onto(figure, (left, top))
        for ball in detections:
            text = f"{ball['colour']}  ({ball['x']}, {ball['y']})  r {ball['radius']}"
            text_width = draw.textlength(text, font=font(15)) + 12
            x = left + (ball["x"] + 0.5) * PANEL_SCALE - text_width / 2
            x = min(max(x, left + 6), left + size - text_width - 6)
            below = top + (ball["y"] + 0.5 + ball["radius"]) * PANEL_SCALE + 9
            above = top + (ball["y"] + 0.5 - ball["radius"]) * PANEL_SCALE - 33
            others = [other for other in detections if other is not ball]
            y = below
            if below + 24 > top + size or any(overlaps(x - left, below - top, text_width, other) for other in others):
                y = above
            chip(draw, x, y, text, size=15, colour=RING[ball["colour"]])

        for column in range(3):
            x = margin + column * (size + gap)
            draw.rectangle([x - 1, top - 1, x + size, top + size], outline=PANEL_EDGE)

    y = height - footer + 14
    draw.text((margin, y), "read from a text dump: the R plane, then G, then B", font=font(15), fill=MUTED)
    draw.text((margin + size + gap, y),
              "dim: inside the RGB thresholds of a colour; bright: largest", font=font(15), fill=MUTED)
    draw.text((margin + size + gap, y + 21),
              "4-connected component; boxed when its radius reaches 13 px", font=font(15), fill=MUTED)
    draw.text((margin + 2 * (size + gap), y),
              "centre and radius derived from the bounding box,", font=font(15), fill=MUTED)
    draw.text((margin + 2 * (size + gap), y + 21),
              "as written to the result file by the C program", font=font(15), fill=MUTED)

    figure.save(path, optimize=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", help="path of the detector (default: PFR or PFR.exe at the repository root)")
    args = parser.parse_args()

    binary = find_binary(args.binary)
    labels = load_ground_truth()
    detections = {name: run_detector(binary, PHOTO_DIR / f"{name}.txt", WORK_DIR / "figures")[0]
                  for name in image_names()}

    DOCS_DIR.mkdir(exist_ok=True)
    contact_sheet(DOCS_DIR / "contact_sheet.png", detections, labels, with_labels=False)
    contact_sheet(DOCS_DIR / "labels.png", detections, labels, with_labels=True)
    pipeline_figure(DOCS_DIR / "pipeline.png", binary, ["IMG_5390", "IMG_5403"])
    for name in ("contact_sheet.png", "labels.png", "pipeline.png"):
        print(f"wrote docs/{name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
