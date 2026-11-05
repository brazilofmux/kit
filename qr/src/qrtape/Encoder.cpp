#include <fstream>
#include <iostream>
#include <cstring>
#include <random>
#include <algorithm>
#include <zlib.h>
#include "Encoder.h"
#include "crc.h"

Encoder::Encoder(const std::string& inputPath) : inputPath(inputPath) {
    generateUuid();
}

void Encoder::generateUuid() {
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (urandom) {
        urandom.read(reinterpret_cast<char*>(fileUuid.data()), fileUuid.size());
    } else {
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (auto& b : fileUuid) b = dist(gen);
    }
}

void Encoder::process(std::function<void(const std::vector<uint8_t>&, size_t, size_t)> callback) {
    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile) {
        throw std::runtime_error("Cannot open input file: " + inputPath);
    }

    uint32_t blockIndex = 0;
    std::vector<uint8_t> buffer(blockSize);

    while (inFile) {
        inFile.read(reinterpret_cast<char*>(buffer.data()), blockSize);
        std::streamsize bytesRead = inFile.gcount();

        if (bytesRead > 0) {
            std::vector<uint8_t> payload(buffer.begin(), buffer.begin() + bytesRead);
            createBlock(blockIndex++, payload, callback);
        }
    }

    // Emit EOF block
    createEofBlock(blockIndex, blockIndex, callback);
}

void Encoder::createBlock(uint32_t blockIndex, const std::vector<uint8_t>& payload,
                          std::function<void(const std::vector<uint8_t>&, size_t, size_t)> callback) {

    // 1. Calculate CRC of original (uncompressed) payload
    uint32_t payloadCrc = CRC32_ProcessBuffer(CRC32_INITIAL, payload.data(), payload.size());
    payloadCrc ^= 0xFFFFFFFF;  // Final XOR for standard CRC32

    // 2. Try to compress the payload
    std::vector<uint8_t> workingPayload;
    bool useCompression = false;
    uint8_t codecId = 0;  // 0 = none, 1 = zlib

    uLongf compressedSize = compressBound(payload.size());
    std::vector<uint8_t> compressed(compressedSize);

    int zret = compress2(compressed.data(), &compressedSize,
                         payload.data(), payload.size(), Z_DEFAULT_COMPRESSION);

    if (zret == Z_OK && compressedSize < payload.size()) {
        // Compression helped - use it
        compressed.resize(compressedSize);
        workingPayload = std::move(compressed);
        useCompression = true;
        codecId = 1;  // zlib
    } else {
        // Compression didn't help or failed - use original
        workingPayload = payload;
    }

    // 3. Calculate FEC on the (possibly compressed) payload
    auto result = fec.encode(workingPayload, FEC_INTERLEAVE_DEPTH);

    // 4. Prepare Block Header
    BlockHeader header;
    std::memcpy(header.magic, BLOCK_MAGIC_BYTES, 4);
    header.version = 1;
    header.flags = FLAG_OUTER_FEC | FLAG_HAS_PAYLOAD_CRC | FLAG_HAS_HEADER_CRC;
    if (useCompression) {
        header.flags |= FLAG_COMPRESSED;
    }

    FECHeaderExt fecExt;
    fecExt.fec_id = 1; // ID 1 = Interleaved RS T=16
    fecExt.codec_id = codecId;
    fecExt.fec_len = static_cast<uint16_t>(result.parity.size());
    fecExt.original_len = static_cast<uint32_t>(payload.size());  // Original uncompressed size
    fecExt.payload_crc32 = payloadCrc;  // CRC of original uncompressed payload

    header.header_len = sizeof(BlockHeader) + sizeof(FECHeaderExt) + sizeof(uint32_t);
    header.file_uuid = fileUuid;
    header.block_index = blockIndex;
    // payload_len is the FEC-padded size of the (possibly compressed) data
    header.payload_len = static_cast<uint32_t>(result.paddedPayload.size());

    // Header CRC excludes the header CRC field itself.
    std::vector<uint8_t> headerBytes;
    headerBytes.reserve(header.header_len);
    const uint8_t* hPtrCrc = reinterpret_cast<const uint8_t*>(&header);
    headerBytes.insert(headerBytes.end(), hPtrCrc, hPtrCrc + sizeof(BlockHeader));
    const uint8_t* ePtrCrc = reinterpret_cast<const uint8_t*>(&fecExt);
    headerBytes.insert(headerBytes.end(), ePtrCrc, ePtrCrc + sizeof(FECHeaderExt));
    uint32_t headerCrc = CRC32_ProcessBuffer(CRC32_INITIAL, headerBytes.data(), headerBytes.size());
    headerCrc ^= 0xFFFFFFFF;
    const uint8_t* cPtr = reinterpret_cast<const uint8_t*>(&headerCrc);
    headerBytes.insert(headerBytes.end(), cPtr, cPtr + sizeof(uint32_t));

    // 3. Serialize Block Data
    // [Payload][Parity]
    std::vector<uint8_t> blockData;
    blockData.reserve(header.payload_len + result.parity.size());
    blockData.insert(blockData.end(), result.paddedPayload.begin(), result.paddedPayload.end());
    blockData.insert(blockData.end(), result.parity.begin(), result.parity.end());

    // 4. Fragment the Block
    if (headerBytes.size() >= fragmentSize) {
        throw std::runtime_error("Header too large for configured fragment size");
    }

    size_t dataChunkSize = fragmentSize - headerBytes.size();
    size_t totalBytes = blockData.size();
    size_t offset = 0;
    uint16_t fragIndex = 0;
    
    uint16_t fragCount = (totalBytes + dataChunkSize - 1) / dataChunkSize;

    while (offset < totalBytes) {
        size_t chunkLen = std::min(dataChunkSize, totalBytes - offset);
        
        FrameHeader fragHeader;
        std::memcpy(fragHeader.magic, FRAME_MAGIC_BYTES, 2);
        fragHeader.version = 1;
        fragHeader.flags = 0;
        fragHeader.file_uuid = fileUuid;
        fragHeader.block_index = blockIndex;
        fragHeader.frag_index = fragIndex;
        fragHeader.frag_count = fragCount;
        fragHeader.frag_payload_len = static_cast<uint16_t>(headerBytes.size() + chunkLen);
        fragHeader.block_header_offset = 0;

        std::vector<uint8_t> qrData;
        qrData.resize(sizeof(FrameHeader) + headerBytes.size() + chunkLen);
        std::memcpy(qrData.data(), &fragHeader, sizeof(FrameHeader));
        std::memcpy(qrData.data() + sizeof(FrameHeader), headerBytes.data(), headerBytes.size());
        std::memcpy(qrData.data() + sizeof(FrameHeader) + headerBytes.size(),
                    blockData.data() + offset, chunkLen);

        callback(qrData, blockIndex, fragIndex);

        offset += chunkLen;
        fragIndex++;
    }
}

void Encoder::createEofBlock(uint32_t blockIndex, uint32_t totalDataBlocks,
                              std::function<void(const std::vector<uint8_t>&, size_t, size_t)> callback) {
    // EOF block payload: just the total data block count
    struct EofPayload {
        uint32_t total_data_blocks;
    } __attribute__((packed));

    EofPayload eofData;
    eofData.total_data_blocks = totalDataBlocks;

    // Prepare Block Header
    BlockHeader header;
    std::memcpy(header.magic, BLOCK_MAGIC_BYTES, 4);
    header.version = 1;
    header.flags = FLAG_EOF_BLOCK | FLAG_HAS_HEADER_CRC;
    header.header_len = sizeof(BlockHeader) + sizeof(uint32_t);
    header.file_uuid = fileUuid;
    header.block_index = blockIndex;
    header.payload_len = sizeof(EofPayload);

    // Serialize Header with CRC (no FEC for EOF block)
    std::vector<uint8_t> headerBytes;
    headerBytes.reserve(header.header_len);
    const uint8_t* hPtr = reinterpret_cast<const uint8_t*>(&header);
    headerBytes.insert(headerBytes.end(), hPtr, hPtr + sizeof(BlockHeader));
    uint32_t headerCrc = CRC32_ProcessBuffer(CRC32_INITIAL, headerBytes.data(), headerBytes.size());
    headerCrc ^= 0xFFFFFFFF;
    const uint8_t* cPtr = reinterpret_cast<const uint8_t*>(&headerCrc);
    headerBytes.insert(headerBytes.end(), cPtr, cPtr + sizeof(uint32_t));

    std::vector<uint8_t> blockData;
    blockData.reserve(sizeof(EofPayload));
    const uint8_t* ePtr = reinterpret_cast<const uint8_t*>(&eofData);
    blockData.insert(blockData.end(), ePtr, ePtr + sizeof(EofPayload));

    // EOF block fits in a single fragment
    FrameHeader fragHeader;
    std::memcpy(fragHeader.magic, FRAME_MAGIC_BYTES, 2);
    fragHeader.version = 1;
    fragHeader.flags = 0;
    fragHeader.file_uuid = fileUuid;
    fragHeader.block_index = blockIndex;
    fragHeader.frag_index = 0;
    fragHeader.frag_count = 1;
    fragHeader.frag_payload_len = static_cast<uint16_t>(headerBytes.size() + blockData.size());
    fragHeader.block_header_offset = 0;

    std::vector<uint8_t> qrData;
    qrData.resize(sizeof(FrameHeader) + headerBytes.size() + blockData.size());
    std::memcpy(qrData.data(), &fragHeader, sizeof(FrameHeader));
    std::memcpy(qrData.data() + sizeof(FrameHeader), headerBytes.data(), headerBytes.size());
    std::memcpy(qrData.data() + sizeof(FrameHeader) + headerBytes.size(),
                blockData.data(), blockData.size());

    callback(qrData, blockIndex, 0);
}
