"""Helpers shared by the evaluation and figure scripts (standard library only).

Every detection used by these scripts comes from the compiled C program: the
helpers below only start it and read back what it wrote.
"""

import csv
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PHOTO_DIR = ROOT / "IMG_300"
GROUND_TRUTH = PHOTO_DIR / "ground_truth.csv"
RESULTS_DIR = ROOT / "results"
DOCS_DIR = ROOT / "docs"
WORK_DIR = ROOT / "build" / "scripts-work"

COLOURS = ("orange", "blue", "yellow")


def find_binary(path=None):
    """Return the path of the detector, or exit with a hint on how to build it."""
    candidates = [Path(path)] if path else [ROOT / "PFR", ROOT / "PFR.exe"]
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    sys.exit("Detector not found: build it first with 'make' (or pass --binary <path>).")


def image_names():
    """Names of the test images (IMG_5389 ... IMG_5408), sorted."""
    return sorted(path.stem for path in PHOTO_DIR.glob("IMG_*.txt"))


def parse_result(path):
    """Read a result file: a count, then one 'colour x y radius' line per ball."""
    lines = Path(path).read_text().split("\n")
    count = int(lines[0])
    detections = []
    for line in lines[1:1 + count]:
        colour, x, y, radius = line.split()
        detections.append({"colour": colour, "x": int(x), "y": int(y), "radius": int(radius)})
    return detections


def run_detector(binary, image_path, work_dir, masks=False):
    """Run the detector on one text image.

    Returns (detections, masks) where masks maps '<colour>_<stage>' to the
    path of the PGM file written by the program (empty unless masks=True).
    """
    work_dir = Path(work_dir)
    work_dir.mkdir(parents=True, exist_ok=True)
    name = Path(image_path).stem
    result_path = work_dir / f"{name}.result.txt"
    command = [str(binary), str(image_path), "-o", str(result_path)]
    prefix = work_dir / f"{name}_"
    if masks:
        for stale in work_dir.glob(f"{name}_*.pgm"):
            stale.unlink()
        command += ["--masks", str(prefix)]
    completed = subprocess.run(command, capture_output=True, text=True)
    if completed.returncode != 0:
        sys.exit(f"{binary.name} failed on {image_path} (exit status {completed.returncode}):\n{completed.stderr}")
    mask_paths = {}
    if masks:
        for path in sorted(work_dir.glob(f"{name}_*.pgm")):
            mask_paths[path.stem[len(name) + 1:]] = path
    return parse_result(result_path), mask_paths


def load_ground_truth(path=GROUND_TRUTH):
    """Return {image name: [ {colour, x, y, radius}, ... ]}; empty scenes map to []."""
    labels = {}
    with open(path, newline="") as handle:
        for row in csv.DictReader(handle):
            balls = labels.setdefault(row["image"], [])
            if row["colour"] != "none":
                balls.append({
                    "colour": row["colour"],
                    "x": int(row["cx"]),
                    "y": int(row["cy"]),
                    "radius": int(row["radius"]),
                })
    return labels


def read_pgm(path):
    """Read a binary PGM (P5, maxval 255) written by the detector.

    Returns (width, height, rows) where rows is a list of bytes objects.
    """
    data = Path(path).read_bytes()
    fields = data.split(maxsplit=4)
    if fields[0] != b"P5" or fields[3] != b"255":
        raise ValueError(f"{path}: not a binary 8-bit PGM")
    width, height = int(fields[1]), int(fields[2])
    pixels = data[len(data) - width * height:]
    return width, height, [pixels[row * width:(row + 1) * width] for row in range(height)]


def mask_bounding_box(rows):
    """Bounding box (min_x, min_y, max_x, max_y) of the non-zero pixels, or None."""
    min_x = min_y = max_x = max_y = None
    for y, row in enumerate(rows):
        stripped = row.lstrip(b"\x00")
        if not stripped:
            continue
        first = len(row) - len(stripped)
        last = len(row.rstrip(b"\x00")) - 1
        min_x = first if min_x is None else min(min_x, first)
        max_x = last if max_x is None else max(max_x, last)
        min_y = y if min_y is None else min_y
        max_y = y
    if min_x is None:
        return None
    return min_x, min_y, max_x, max_y
