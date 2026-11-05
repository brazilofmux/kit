#!/bin/bash

# Layout QR Tape images into a PDF grid using ImageMagick 'montage'.

OUTPUT_PDF="qr_tape.pdf"
INPUT_DIR="."

if [ -n "$1" ]; then
    INPUT_DIR="$1"
fi

if [ -n "$2" ]; then
    OUTPUT_PDF="$2"
fi

if ! command -v montage &> /dev/null; then
    echo "Error: 'montage' (ImageMagick) is not installed."
    exit 1
fi

echo "Searching for PPM files in $INPUT_DIR..."

# Find files matching the pattern and store in array for safe handling
mapfile -t FILES < <(find "$INPUT_DIR" -name "out_B*_F*.ppm" -print0 | sort -z | xargs -0 -n1)

if [ ${#FILES[@]} -eq 0 ]; then
    echo "No matching PPM files found."
    exit 1
fi

echo "Found ${#FILES[@]} fragments. Generating PDF..."

# Montage options:
# -tile 2x4: 2 columns, 4 rows (8 per page)
# -geometry +20+20: No resize (use original QR size), with 20px padding
# -label "%t": Use filename (without extension) as label
# -pointsize 12: Label font size
# -frame 5: Add a small frame
# -page A4: A4 page size

# Detect a usable font, skip labels if none found
LABEL_OPTS=()
FONT_FILE=$(fc-match --format='%{file}' monospace 2>/dev/null)
if [ -n "$FONT_FILE" ] && [ -f "$FONT_FILE" ]; then
    LABEL_OPTS=(-font "$FONT_FILE" -label "%t" -pointsize 12)
fi

montage "${FILES[@]}" \
    -tile 2x4 \
    -geometry +20+20 \
    "${LABEL_OPTS[@]}" \
    -frame 2 \
    -page A4 \
    "$OUTPUT_PDF"

if [ $? -ne 0 ]; then
    echo "Failed to create PDF."
    exit 1
fi

# Verify the output is actually a PDF (not a silently-wrong format)
MAGIC=$(head -c 5 "$OUTPUT_PDF" 2>/dev/null)
if [ "$MAGIC" != "%PDF-" ]; then
    echo "Error: Output file is not a valid PDF."
    echo "ImageMagick may be missing PDF/Ghostscript support."
    echo "Install ghostscript (e.g., 'apk add ghostscript' or 'apt install ghostscript')."
    rm -f "$OUTPUT_PDF"
    exit 1
fi

echo "Successfully created $OUTPUT_PDF"
