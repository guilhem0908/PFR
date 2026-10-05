#!/bin/sh
# Regression tests for the detector binary.
#
# Usage: tests/run_tests.sh <detector> <gen_image>
#
#   <detector>   the PFR executable to test
#   <gen_image>  the synthetic image generator built from tests/gen_image.c
#
# Three groups of checks:
#   1. the 20 photos of IMG_300/ against the outputs stored in tests/expected/;
#   2. synthetic scenes whose expected output follows from their geometry;
#   3. command-line and malformed-input handling.
#
# Temporary files go to build/test-work/ at the root of the repository.

set -u

if [ $# -ne 2 ]; then
    echo "Usage: $0 <detector> <gen_image>" >&2
    exit 2
fi

absolute() {
    echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
}

BIN=$(absolute "$1")
GEN=$(absolute "$2")
ROOT=$(cd "$(dirname "$0")/.." && pwd)

cd "$ROOT" || exit 2
WORK=build/test-work
rm -rf "$WORK"
mkdir -p "$WORK" || exit 2

passed=0
failed=0

pass() {
    passed=$((passed + 1))
}

fail() {
    failed=$((failed + 1))
    echo "FAIL: $1"
    if [ -s "$WORK/stderr.txt" ]; then
        sed 's/^/    stderr: /' "$WORK/stderr.txt"
    fi
}

# run <args...>: runs the detector, keeps its exit status in $status and its
# streams in $WORK/stdout.txt and $WORK/stderr.txt.
run() {
    rm -f "$WORK/out.txt"
    "$BIN" "$@" > "$WORK/stdout.txt" 2> "$WORK/stderr.txt"
    status=$?
    if grep -q -e "Sanitizer" -e "runtime error" "$WORK/stderr.txt"; then
        status=99
    fi
}

# Content of a text file on one line, without carriage returns.
flat() {
    tr -d '\r' < "$1" | tr '\n' '|'
}

# expect_result <name> <expected, lines joined with '|'> <detector args...>
expect_result() {
    name=$1
    expected=$2
    shift 2
    run "$@" -o "$WORK/out.txt"
    if [ "$status" -ne 0 ]; then
        fail "$name (exit status $status)"
    elif [ "$(flat "$WORK/out.txt")" != "$expected" ]; then
        fail "$name (got '$(flat "$WORK/out.txt")', expected '$expected')"
    else
        pass
    fi
}

# expect_status <name> <expected exit status> <detector args...>
# A failing run must not leave a result file behind.
expect_status() {
    name=$1
    expected=$2
    shift 2
    run "$@"
    if [ "$status" -ne "$expected" ]; then
        fail "$name (exit status $status, expected $expected)"
    elif [ "$expected" -ne 0 ] && [ -e "$WORK/out.txt" ]; then
        fail "$name (a result file was written)"
    else
        pass
    fi
}

# ---------------------------------------------------------------------------
# 1. The 20 photos
# ---------------------------------------------------------------------------

photos=0
for expected_file in tests/expected/IMG_*.txt; do
    name=$(basename "$expected_file" .txt)
    photos=$((photos + 1))
    expect_result "photo $name" "$(flat "$expected_file")" "IMG_300/$name.txt"
done
if [ "$photos" -ne 20 ]; then
    fail "expected 20 photo references in tests/expected, found $photos"
fi

# The default result file is result.txt in the current directory.
(cd "$WORK" && "$BIN" "$ROOT/IMG_300/IMG_5392.txt" > /dev/null 2>&1)
if [ "$(flat "$WORK/result.txt" 2> /dev/null)" = "$(flat tests/expected/IMG_5392.txt)" ]; then
    pass
else
    fail "default output file result.txt"
fi

# ---------------------------------------------------------------------------
# 2. Synthetic scenes
#
# gen_image paints filled discs on a grey background. The bounding box of a
# disc of radius r centred on (cx, cy) is [cx - r, cx + r] x [cy - r, cy + r],
# so the detector must report centre (cx, cy) and radius r for a blue ball,
# and for orange and yellow balls the same values after the corrections:
#   orange: y -> y * (10000 - 13 r) / 10000,  r -> r * 112 / 100
#   yellow: y -> y * (10000 + 15 r) / 10000,  r -> r * 112 / 100
# ---------------------------------------------------------------------------

ORANGE="200 50 30"
BLUE="20 60 150"
YELLOW="220 200 50"

scene() {
    out=$1
    shift
    "$GEN" "$@" > "$WORK/$out" || fail "gen_image $out"
}

scene blue.txt 300 300 100 80 20 $BLUE
expect_result "blue disc" "1|blue 100 80 20|" "$WORK/blue.txt"

scene orange.txt 300 300 120 150 30 $ORANGE
expect_result "orange disc" "1|orange 120 144 33|" "$WORK/orange.txt"

# 50 * (1 + 0.0015 * 40) is exactly 53: floating-point builds disagreed here.
scene yellow.txt 300 300 150 50 40 $YELLOW
expect_result "yellow disc, whole-number correction" "1|yellow 150 53 44|" "$WORK/yellow.txt"

scene three.txt 300 300 60 60 20 $YELLOW 240 70 25 $BLUE 150 200 50 $ORANGE
expect_result "three balls" "3|yellow 60 61 22|blue 240 70 25|orange 150 187 56|" "$WORK/three.txt"

scene radius12.txt 300 300 150 150 12 $BLUE
expect_result "radius 12 is rejected" "0|" "$WORK/radius12.txt"

scene radius13.txt 300 300 150 150 13 $BLUE
expect_result "radius 13 is kept" "1|blue 150 150 13|" "$WORK/radius13.txt"

scene two_blue.txt 300 300 70 70 20 $BLUE 200 200 30 $BLUE
expect_result "largest of two blobs of one colour" "1|blue 200 200 30|" "$WORK/two_blue.txt"

# A small blob in the middle of the list used to free the clusters after it.
scene small_between.txt 300 300 80 80 40 $YELLOW 200 100 8 $BLUE 200 220 40 $ORANGE
expect_result "small blob between two balls" "2|yellow 80 84 44|orange 200 208 44|" "$WORK/small_between.txt"

# One 90,000-pixel component: the recursive flood fill overflowed the stack.
scene full_frame.txt 300 300 150 150 400 $ORANGE
expect_result "full-frame blob" "1|orange 149 120 166|" "$WORK/full_frame.txt"

scene wide.txt 120 60 90 30 20 $BLUE
expect_result "non-square image" "1|blue 90 30 20|" "$WORK/wide.txt"

scene tall.txt 60 120 30 90 20 $BLUE
expect_result "non-square image, tall" "1|blue 30 90 20|" "$WORK/tall.txt"

scene grey.txt 300 300
expect_result "no ball" "0|" "$WORK/grey.txt"

scene pixel.txt 1 1
expect_result "1 x 1 image" "0|" "$WORK/pixel.txt"

awk '{ printf "%s\r\n", $0 }' "$WORK/blue.txt" > "$WORK/blue_crlf.txt"
expect_result "CRLF line endings" "1|blue 100 80 20|" "$WORK/blue_crlf.txt"

tr '\n' ' ' < "$WORK/blue.txt" > "$WORK/blue_one_line.txt"
expect_result "image on a single line" "1|blue 100 80 20|" "$WORK/blue_one_line.txt"

# Mask export: one PGM per colour and per stage, 15 header bytes + one byte per pixel.
white_pixels() {
    tail -c 90000 "$1" | tr -d '\000' | wc -c | tr -d ' '
}

run "$WORK/two_blue.txt" -o "$WORK/out.txt" --masks "$WORK/mask_"
if [ "$status" -ne 0 ]; then
    fail "mask export (exit status $status)"
elif [ "$(ls "$WORK" | grep -c '^mask_')" -ne 2 ]; then
    fail "mask export (expected 2 files, found: $(ls "$WORK" | grep '^mask_' | tr '\n' ' '))"
elif [ "$(head -c 15 "$WORK/mask_blue_largest.pgm" | tr '\n' ' ')" != "P5 300 300 255 " ]; then
    fail "mask export (PGM header)"
elif [ "$(wc -c < "$WORK/mask_blue_threshold.pgm" | tr -d ' ')" -ne 90015 ]; then
    fail "mask export (file size)"
elif [ "$(white_pixels "$WORK/mask_blue_threshold.pgm")" -ne 4078 ]; then
    # 1,257 pixels in the disc of radius 20 and 2,821 in the disc of radius 30
    fail "mask export (threshold mask has $(white_pixels "$WORK/mask_blue_threshold.pgm") white pixels, expected 4078)"
elif [ "$(white_pixels "$WORK/mask_blue_largest.pgm")" -ne 2821 ]; then
    fail "mask export (largest component has $(white_pixels "$WORK/mask_blue_largest.pgm") white pixels, expected 2821)"
else
    pass
fi

# ---------------------------------------------------------------------------
# 3. Command line and malformed input
# ---------------------------------------------------------------------------

expect_status "no argument" 2
expect_status "unknown option" 2 --frobnicate "$WORK/blue.txt"
expect_status "two images" 2 "$WORK/blue.txt" "$WORK/grey.txt"
expect_status "missing value for -o" 2 "$WORK/blue.txt" -o
expect_status "missing value for --masks" 2 "$WORK/blue.txt" --masks
expect_status "help" 0 --help

run
if grep -q "Usage:" "$WORK/stderr.txt"; then pass; else fail "usage message on stderr"; fi

expect_status "missing file" 1 "$WORK/does_not_exist.txt" -o "$WORK/out.txt"
expect_status "unwritable result file" 1 "$WORK/blue.txt" -o "$WORK/no_such_directory/out.txt"

bad() {
    name=$1
    printf '%s' "$2" > "$WORK/bad.txt"
    expect_status "$name" 1 "$WORK/bad.txt" -o "$WORK/out.txt"
}

bad "empty file" ""
bad "incomplete header" "300 300"
bad "greyscale image" "2 2 1 1 2 3 4"
bad "zero width" "0 2 3"
bad "negative height" "2 -2 3"
bad "side above the limit" "5000 2 3"
bad "missing sample" "2 1 3 1 2 3 4 5"
bad "sample above 255" "1 1 3 0 0 256"
bad "negative sample" "1 1 3 0 -1 0"
bad "text instead of a number" "1 1 3 12 abc 3"
bad "binary garbage" "$(printf 'P6\001\002\377\376')"

# A photo cut in the middle of the blue plane.
head -c 800000 IMG_300/IMG_5390.txt > "$WORK/truncated.txt"
expect_status "truncated photo" 1 "$WORK/truncated.txt" -o "$WORK/out.txt"

# ---------------------------------------------------------------------------

echo "regression tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
