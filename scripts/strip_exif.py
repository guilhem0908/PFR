#!/usr/bin/env python3
"""Remove camera metadata from the JPEG test photos without re-encoding them.

A JPEG file is a sequence of segments. This script drops the ones that carry
metadata (APP1: Exif and XMP, APP13: IPTC, COM: comments) and copies every
other byte unchanged, so the compressed image data, and therefore every
decoded pixel, stays identical. The ICC colour profile (APP2) is kept.

    python scripts/strip_exif.py            # rewrite IMG_300/*.jpeg in place
    python scripts/strip_exif.py --check    # exit 1 if a photo still has metadata

Only the standard library is used.
"""

import argparse
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PHOTO_DIR = ROOT / "IMG_300"

SOI = 0xD8
SOS = 0xDA
METADATA_MARKERS = {0xE1: "APP1 (Exif/XMP)", 0xED: "APP13 (IPTC)", 0xFE: "COM (comment)"}


def split_segments(data):
    """Return (header segments, rest of the file starting at the scan data).

    Each header segment is a tuple (marker, raw bytes including the marker).
    """
    if data[:2] != bytes([0xFF, SOI]):
        raise ValueError("not a JPEG file")
    segments = []
    position = 2
    while position < len(data):
        if data[position] != 0xFF:
            raise ValueError(f"unexpected byte at offset {position}")
        marker = data[position + 1]
        if marker == SOS:
            return segments, data[position:]
        (length,) = struct.unpack(">H", data[position + 2:position + 4])
        segments.append((marker, data[position:position + 2 + length]))
        position += 2 + length
    raise ValueError("no scan data found")


def strip(data):
    """Return (stripped bytes, names of the removed segments)."""
    segments, rest = split_segments(data)
    kept = [raw for marker, raw in segments if marker not in METADATA_MARKERS]
    removed = [METADATA_MARKERS[marker] for marker, _ in segments if marker in METADATA_MARKERS]
    return bytes([0xFF, SOI]) + b"".join(kept) + rest, removed


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="report metadata without modifying any file")
    parser.add_argument("photos", nargs="*", type=Path, help="JPEG files (default: IMG_300/*.jpeg)")
    args = parser.parse_args()

    photos = args.photos or sorted(PHOTO_DIR.glob("*.jpeg"))
    if not photos:
        print("no photo found", file=sys.stderr)
        return 1

    with_metadata = 0
    for photo in photos:
        data = photo.read_bytes()
        stripped, removed = strip(data)
        if not removed:
            continue
        with_metadata += 1
        if args.check:
            print(f"{photo.name}: {', '.join(removed)}")
        else:
            photo.write_bytes(stripped)
            print(f"{photo.name}: removed {', '.join(removed)} ({len(data) - len(stripped)} bytes)")

    if args.check:
        print(f"{len(photos)} photos checked, {with_metadata} with metadata")
        return 1 if with_metadata else 0
    print(f"{len(photos)} photos processed, {with_metadata} rewritten")
    return 0


if __name__ == "__main__":
    sys.exit(main())
