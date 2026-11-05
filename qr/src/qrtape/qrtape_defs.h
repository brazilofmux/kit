#ifndef QRTAPE_DEFS_H
#define QRTAPE_DEFS_H

#include <cstdint>
#include <array>

// Magic constants
constexpr uint32_t QRTAPE_BLOCK_MAGIC = 0x42545251; // "QRTB" in little-endian (B T R Q) -> Q R T B ? 
// Actually: "QRTB" is 0x51 0x52 0x54 0x42.
// Little endian: 42 54 52 51. 
// Let's use char array for safety or explicit bytes.
constexpr char BLOCK_MAGIC_BYTES[4] = {'Q', 'R', 'T', 'B'};
constexpr char FRAME_MAGIC_BYTES[2] = {'Q', 'F'};

// UUID type (16 bytes)
using UUID = std::array<uint8_t, 16>;

// Block Header
struct __attribute__((packed)) BlockHeader {
    uint8_t magic[4];       // "QRTB"
    uint8_t version;
    uint8_t flags;
    uint16_t header_len;
    UUID file_uuid;
    uint32_t block_index;
    uint32_t payload_len;   // Bytes after header, before outer FEC
};

// Block Flags
// These flags indicate which features are enabled for a block.
enum BlockFlags : uint8_t {
    FLAG_COMPRESSED     = 0x01,  // Payload is compressed (codec_id in FECHeaderExt)
    FLAG_ENCRYPTED      = 0x02,  // Reserved for future encryption support (NOT IMPLEMENTED)
    FLAG_OUTER_FEC      = 0x04,  // Block has FEC parity appended
    FLAG_EOF_BLOCK      = 0x08,  // This is an EOF marker block
    FLAG_HAS_PAYLOAD_CRC= 0x10,  // Payload CRC32 is present in extended header
    FLAG_HAS_HEADER_CRC = 0x20   // Header CRC32 is appended after extended header
};

// Fragment (Frame) Header
struct __attribute__((packed)) FrameHeader {
    uint8_t magic[2];       // "QF"
    uint8_t version;
    uint8_t flags;
    UUID file_uuid;
    uint32_t block_index;
    uint16_t frag_index;
    uint16_t frag_count;
    uint16_t frag_payload_len;
    uint16_t block_header_offset;
};

// FEC Configuration
constexpr int FEC_INTERLEAVE_DEPTH = 32;

// FEC Extended Header (12 bytes)
// Follows BlockHeader when FLAG_OUTER_FEC is set
struct __attribute__((packed)) FECHeaderExt {
    uint8_t fec_id;         // FEC algorithm ID (1 = Interleaved RS T=16)
    uint8_t codec_id;       // Compression codec ID (0 = none, 1 = zlib)
    uint16_t fec_len;       // Length of parity data in bytes
    uint32_t original_len;  // Original uncompressed payload size
    uint32_t payload_crc32; // CRC32 of original uncompressed payload
};

#endif // QRTAPE_DEFS_H
