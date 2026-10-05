#!/usr/bin/env python3
"""Time the detector on the 20 test photos and write results/timing.json.

Each measurement covers one whole run of the program: process start-up,
reading and parsing the 1 MB text image, detection, and writing the result
file. Every photo is run several times and its fastest run is kept; the
figures depend on the machine, so the file records where it was measured.

    python scripts/benchmark.py [--runs 7]

Only the standard library is used.
"""

import argparse
import json
import platform
import statistics
import subprocess
import sys
import time

from common import PHOTO_DIR, RESULTS_DIR, WORK_DIR, find_binary, image_names


def cpu_name():
    """Human-readable processor name, when the system exposes one."""
    try:
        if platform.system() == "Windows":
            import winreg
            key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0")
            return winreg.QueryValueEx(key, "ProcessorNameString")[0].strip()
        with open("/proc/cpuinfo") as handle:
            for line in handle:
                if line.startswith("model name"):
                    return line.split(":", 1)[1].strip()
    except OSError:
        pass
    return platform.processor() or platform.machine()


def compiler_version():
    try:
        return subprocess.run(["gcc", "--version"], capture_output=True, text=True).stdout.splitlines()[0]
    except (OSError, IndexError):
        return "unknown"


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", help="path of the detector (default: PFR or PFR.exe at the repository root)")
    parser.add_argument("--runs", type=int, default=7, help="runs per photo (default: 7)")
    args = parser.parse_args()

    binary = find_binary(args.binary)
    work = WORK_DIR / "benchmark"
    work.mkdir(parents=True, exist_ok=True)

    per_image = {}
    for name in image_names():
        command = [str(binary), str(PHOTO_DIR / f"{name}.txt"), "-o", str(work / "result.txt")]
        timings = []
        for _ in range(args.runs):
            start = time.perf_counter()
            subprocess.run(command, check=True, capture_output=True)
            timings.append((time.perf_counter() - start) * 1000)
        per_image[name] = round(min(timings), 1)

    values = list(per_image.values())
    report = {
        "what": "wall-clock time of one run of the detector on one 300 x 300 text image, in milliseconds "
                "(process start-up, parsing, detection, result file); fastest of several runs per photo",
        "runs_per_photo": args.runs,
        "cpu": cpu_name(),
        "system": f"{platform.system()} {platform.release()} ({platform.machine()})",
        "compiler": compiler_version(),
        "median_ms": round(statistics.median(values), 1),
        "min_ms": min(values),
        "max_ms": max(values),
        "per_photo_ms": per_image,
    }
    RESULTS_DIR.mkdir(exist_ok=True)
    (RESULTS_DIR / "timing.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"median {report['median_ms']} ms per photo (min {report['min_ms']}, max {report['max_ms']}) "
          f"on {report['cpu']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
