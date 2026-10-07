#!/usr/bin/env python3
"""Check that the numbers quoted in the prose of README.md match the repository.

The tables between the 'results' markers are checked by scripts/evaluate.py
--check. This script covers the numbers written in sentences: the size of the
labelled set, the test counts, the timings, the worst cases and the statements
about brightness. It reads the committed files in results/ and IMG_300/, and
runs the two test programs built by 'make test' to count their checks.

    python scripts/check_readme.py

Exit status 1 when a sentence disagrees with the data. Standard library only.
"""

import csv
import json
import re
import subprocess
import sys

from common import RESULTS_DIR, ROOT

README = ROOT / "README.md"
BUILD = ROOT / "build"
UNIT_LINE = re.compile(r"unit tests: (\d+) checks, (\d+) failed")
REGRESSION_LINE = re.compile(r"regression tests: (\d+) passed, (\d+) failed")


def executable(directory, name):
    """Path of a built program, with or without the Windows suffix."""
    for candidate in (directory / name, directory / f"{name}.exe"):
        if candidate.is_file():
            return candidate
    sys.exit(f"{name} is not built: run 'make test' first.")


def count_checks():
    """Run the unit tests and the regression script; return their check counts."""
    detector = executable(ROOT, "PFR")
    generator = executable(BUILD, "gen_image")
    units = subprocess.run([str(executable(BUILD, "test_units"))], capture_output=True, text=True, cwd=ROOT)
    regression = subprocess.run(["sh", "tests/run_tests.sh", str(detector), str(generator)],
                                capture_output=True, text=True, cwd=ROOT)
    unit_match = UNIT_LINE.search(units.stdout + units.stderr)
    regression_match = REGRESSION_LINE.search(regression.stdout + regression.stderr)
    if not unit_match or not regression_match:
        sys.exit("could not read the check counts printed by the test programs")
    return int(unit_match.group(1)), int(regression_match.group(1))


def read_csv(path):
    with open(path, newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def expected_sentences():
    """Every sentence fragment the README must contain, with the data it comes from."""
    summary = json.loads((RESULTS_DIR / "summary.json").read_text(encoding="utf-8"))
    dataset, detection = summary["dataset"], summary["detection"]
    rows = [row for row in read_csv(RESULTS_DIR / "detections.csv") if row["verdict"] == "found"]
    linux = json.loads((RESULTS_DIR / "timing_linux.json").read_text(encoding="utf-8"))
    windows = json.loads((RESULTS_DIR / "timing_windows.json").read_text(encoding="utf-8"))
    brightness = read_csv(RESULTS_DIR / "brightness.csv")
    units, regression = count_checks()

    worst_centre = max(rows, key=lambda row: float(row["centre_error_px"]))
    worst_radius = min(rows, key=lambda row: int(row["radius_error_px"]))
    cpu = re.sub(r"\((R|TM)\)", "", linux["cpu"]).replace("  ", " ")
    gcc_linux = re.search(r"\) (\d+)\.", linux["compiler"]).group(1)
    gcc_windows = re.search(r"GCC-(\d+\.\d+)", windows["compiler"]).group(1)
    by_gain = {int(row["gain_percent"]): row for row in brightness}
    lost_either_side = all(int(by_gain[gain]["balls_found"]) < dataset["balls"] for gain in (90, 110))
    yellow_gone_at_70 = all(int(by_gain[gain]["yellow_found"]) == 0 for gain in (50, 60, 70))
    false_on_both_sides = (any(int(row["false_detections"]) > 0 for gain, row in by_gain.items() if gain < 100)
                           and any(int(row["false_detections"]) > 0 for gain, row in by_gain.items() if gain > 100))
    box_offset = {colour: abs(summary["corrections"][colour]["bounding_box"]["vertical_offset_mean_px"])
                  for colour in ("orange", "yellow")}

    claims = [
        (f"{dataset['balls']} balls over {dataset['images_with_balls']} photos", "size of the labelled set"),
        (f"{dataset['empty_scenes']} photos without any ball", "empty scenes"),
        (f"{units} unit checks", "unit test count"),
        (f"{regression} checks on the compiled program", "regression test count"),
        (f"{worst_centre['image']}, {worst_centre['colour']}, {float(worst_centre['centre_error_px']):.0f} px off",
         "worst centre error"),
        (f"{worst_radius['image']}, {worst_radius['colour']}, "
         f"radius {-int(worst_radius['radius_error_px'])} px too small", "worst radius error"),
        (f"about {round(linux['median_ms'])} ms per image in a Linux container (gcc {gcc_linux})", "Linux timing"),
        (f"about {round(windows['median_ms'])} ms on Windows (MinGW gcc {gcc_windows})", "Windows timing"),
        (cpu, "processor name"),
        (f"{detection['balls_found']} balles trouvées sur {detection['balls']}", "French summary"),
    ]
    statements = [
        (lost_either_side, "a 10 % brightness change loses a ball on both sides"),
        (yellow_gone_at_70, "no yellow ball found at 70 % or below"),
        (false_on_both_sides, "false detections appear on both sides"),
        (all(value > 10 for value in box_offset.values()), "bounding box more than 10 px off vertically"),
        (detection["false_detections"] == 0 and detection["balls_missed"] == 0, "perfect score on the 20 photos"),
    ]
    return claims, statements


def main():
    text = " ".join(README.read_text(encoding="utf-8").split())
    claims, statements = expected_sentences()
    failures = [f"missing from README.md: '{fragment}' ({source})"
                for fragment, source in claims if fragment not in text]
    failures += [f"not supported by the results: {meaning}" for holds, meaning in statements if not holds]
    for line in failures:
        print(line)
    if failures:
        sys.exit(1)
    print(f"{len(claims)} figures and {len(statements)} statements of the README match the repository.")


if __name__ == "__main__":
    main()
