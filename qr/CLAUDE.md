# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

QR Tape is a storage format for encoding arbitrary binary data as sequences of QR codes, inspired by tape storage concepts. It supports forward/backward scanning, compression (zlib), CRC32 verification, and Reed-Solomon forward error correction (FEC).

## Build Commands

### Build librs (Reed-Solomon library)
```bash
cd librs && make
```
Produces `librs.a` static library.

### Build qrtape tools
```bash
cd src/qrtape && make
```
Produces:
- `qrtapemake` - Encodes files into QR code fragments (PPM images)
- `qrtaperead` - Decodes QR code fragments back to files

### Clean
```bash
cd src/qrtape && make clean
```

### Run FEC Test
```bash
cd src/qrtape && ./test_fec.sh [size_in_bytes]
```
Tests encoding, simulates fragment loss, verifies FEC recovery.

### Generate PDF from QR Fragments
```bash
cd src/qrtape && ./layout_tape.sh [input_dir] [output.pdf]
```
Requires ImageMagick `montage`.

### Read from PDF
```bash
cd src/qrtape && ./read_pdf.sh <input.pdf> <output_file>
```
Requires poppler-utils (`pdftoppm`).

## Dependencies

- C++17 compiler (g++)
- libqrencode (QR code generation)
- libzbar (QR code reading from images)
- zlib (compression)
- poppler-utils (optional, for PDF input)
- ImageMagick (optional, for PDF output)

## Architecture

### Data Flow

```
Input File → [Compress] → [CRC32] → [FEC Encode] → Blocks → Fragments → QR Codes (PPM)
```

### Key Components

**librs/** - Reed-Solomon codec library
- `RS_FACTORY` - Creates encoder/decoder instances with configurable T (correction capability)
- Optimized encoders for common T values (t1, t2, t4, t8, t14, t16, t32, t64)
- Uses GF(2^8) with N=255 codeword size
- CRC32 implementation

**src/qrtape/** - Main application
- `Encoder` - Compresses, calculates CRC, applies FEC, fragments to QR size, outputs PPM
- `Decoder` - Reads QR images via zbar, reassembles, applies FEC correction, decompresses, verifies CRC
- `InterleavedRS` (RS.h) - Wraps librs with interleaving (depth=32) to spread burst errors
- `qrtape_defs.h` - Binary format structures and flags

### FEC Design

Uses interleaved Reed-Solomon (T=16, corrects up to 16 erasures per codeword):
- Data striped across 32 codewords (`FEC_INTERLEAVE_DEPTH`)
- Missing QR frames become erasures spread across all codewords
- Erasure decoding (known positions) doubles correction capability vs error decoding

### Binary Format

**Block Header** (32 bytes): magic "QRTB", version, flags, header_len, UUID, block_index, payload_len

**Extended Header** (12 bytes, when FEC enabled):
- fec_id (1) + codec_id (1) + fec_len (2) + original_len (4) + payload_crc32 (4)

**Frame Header** (30 bytes): magic "QF", version, flags, UUID, block_index, frag_index, frag_count, payload_len

**Flags:**
- 0x01: compressed (zlib)
- 0x02: encrypted (reserved, not implemented)
- 0x04: outer_fec
- 0x08: eof_block
- 0x10: has_payload_crc
- 0x20: has_header_crc

**EOF Block:** Contains total data block count; used for completeness detection.
