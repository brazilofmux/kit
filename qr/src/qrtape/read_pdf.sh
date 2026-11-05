#!/bin/bash

# Read QR Tape data from a PDF file.
# Requires: poppler-utils (pdftoppm), qrtaperead

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <input.pdf> <output_file>" >&2
    exit 1
fi

INPUT_PDF="$1"
OUTPUT_FILE="$2"

if [ ! -f "$INPUT_PDF" ]; then
    echo "Error: PDF file not found: $INPUT_PDF" >&2
    exit 1
fi

if ! command -v pdftoppm &> /dev/null; then
    echo "Error: 'pdftoppm' not found. Install poppler-utils." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
QRTAPEREAD="$SCRIPT_DIR/qrtaperead"

if [ ! -x "$QRTAPEREAD" ]; then
    echo "Error: qrtaperead not found or not executable at $QRTAPEREAD" >&2
    exit 1
fi

# Create temp directory for extracted images
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

echo "Extracting pages from PDF..."
pdftoppm -r 300 "$INPUT_PDF" "$TMPDIR/page"

# Count extracted pages
PAGE_COUNT=$(ls "$TMPDIR"/page-*.ppm 2>/dev/null | wc -l)
if [ "$PAGE_COUNT" -eq 0 ]; then
    echo "Error: No pages extracted from PDF." >&2
    exit 1
fi

echo "Extracted $PAGE_COUNT page(s). Decoding QR codes..."

# Run qrtaperead on extracted images
"$QRTAPEREAD" "$OUTPUT_FILE" "$TMPDIR"/page-*.ppm
