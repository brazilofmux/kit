# QR Tape Design (Draft)

This document proposes a storage format for representing files as a sequence of QR codes, borrowing block ideas from tape formats while staying binary-first and UTF-8 friendly.

## Goals

- Encode arbitrary binary data into QR symbols without assuming file size up front.
- Support forward and backward scanning, like tape.
- Allow optional compression, encryption, and outer forward error correction (FEC).
- Keep parsing deterministic with fixed headers and explicit lengths.

## Terminology

- **Block**: The fundamental logical unit of storage. A block contains a header, payload (data), and optional checksums/FEC.
- **Fragment (Frame)**: The physical unit of storage corresponding to a single QR symbol. Large blocks are split into multiple fragments.
- **Dataset**: A sequence of blocks (ending with an EOF block) representing one or more files.

## Model Overview

Data is split into blocks. Each block may span one or more QR symbols (fragments). A block is self-describing, and blocks can be re-ordered using their indices if scans arrive out of order.

Pipeline: raw bytes -> optional compression -> optional encryption -> optional outer FEC -> block framing -> QR frames.

## Record and Block Rules

- Input is treated as bytes. Text should be UTF-8 encoded outside this format.
- Blocks are fixed-size in bytes, except the final block which may be shorter.
- Each block has a header that includes a UUID, block index, payload length, and checksums.
- A special EOF block acts as a filemark and includes final metadata.

## Block Header (binary, little endian)

Minimum header (32 bytes):

- magic: 4 bytes, "QRTB"
- version: 1 byte
- flags: 1 byte (bitfield)
- header_len: 2 bytes
- file_uuid: 16 bytes
- block_index: 4 bytes (0-based)
- payload_len: 4 bytes (bytes after header, before outer FEC)

Optional fields (indicated by header_len and flags):

- codec_id: 1 byte (compression)
- cipher_id: 1 byte (encryption)
- fec_id: 1 byte (outer FEC)
- fec_len: 2 bytes (bytes of outer FEC appended to payload)
- payload_crc32: 4 bytes (CRC of payload before outer FEC)
- header_crc32: 4 bytes (CRC of header excluding this field)

Flags (draft):

- 0x01: compressed
- 0x02: encrypted
- 0x04: outer_fec
- 0x08: eof_block
- 0x10: has_payload_crc
- 0x20: has_header_crc

## QR Frame Header (binary, per symbol)

Each QR symbol contains:

- frame_magic: 2 bytes, "QF"
- version: 1 byte
- frame_flags: 1 byte
- file_uuid: 16 bytes
- block_index: 4 bytes
- frag_index: 2 bytes (0-based)
- frag_count: 2 bytes (total fragments in this block)
- frag_payload_len: 2 bytes
- block_header_offset: 2 bytes (0 for first fragment, else offset into block header)

Each fragment includes a copy of the block header (offset 0) followed by its data payload bytes. This redundancy allows reconstruction even if fragment 0 is missing.

## Forward/Backward Scanning

- Each frame includes `block_index` and `frag_index`, so fragments can be reassembled regardless of scan order.
- EOF block marks a complete dataset. Readers can stop when EOF is seen or continue to scan for additional datasets.

## Compression, Encryption, and FEC

- Compression applies before encryption. Encryption applies before outer FEC.
- QR has its own internal ECC (handling local damage like scratches).
- **Outer FEC** is optional and intended to handle **missing QR frames** (burst erasures).

### Outer FEC and Striping

To robustly handle missing frames, Outer FEC uses **Interleaved Reed-Solomon**:

1.  **Striping**: Codewords are striped (interleaved) across the block payload. If the interleave depth is `D`, byte `i` of the block belongs to RS codeword `i % D`.
2.  **Burst Tolerance**: A missing QR frame (a contiguous burst of errors) is distributed across all `D` codewords.
    - Example: If a frame holds 500 bytes and we use 10 interleaved codewords, a missing frame results in only ~50 missing bytes per codeword.
3.  **Erasure Decoding**: Since the block headers in surviving fragments identify exactly which fragment is missing, the decoder treats these as *erasures* (known locations), doubling the correction capability of the RS code compared to unknown errors.

- Cipher and compression identifiers are numeric and resolved by an external registry.

## QR Capacity Considerations

- QR payload sizes vary by version and ECC level; the encoder must select a level and compute `frag_payload_len` accordingly.
- If data exceeds a single symbol, use multiple frames with consistent `block_index` and `frag_count`.

## Open Questions / Next Steps

- Choose default block size and typical QR version/ECC level.
- Define exact codec/cipher/FEC registries (initial values).
- Decide whether to include a global dataset header block before block 0.
