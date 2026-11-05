#include "Decoder.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <limits>
#include <cctype>
#include <zbar.h>
#include <zlib.h>
#include "crc.h"

Decoder::Decoder() {}

static bool readPpmToken(std::istream& in, std::string& out) {
    out.clear();
    char ch = '\0';
    while (in.get(ch)) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            continue;
        }
        if (ch == '#') {
            in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        out.push_back(ch);
        break;
    }
    if (out.empty()) return false;

    while (in.get(ch)) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            break;
        }
        if (ch == '#') {
            in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        out.push_back(ch);
    }
    return true;
}

bool Decoder::addFragment(const std::vector<uint8_t>& fragData) {
    if (fragData.size() < sizeof(FrameHeader)) return false;

    FrameHeader header;
    std::memcpy(&header, fragData.data(), sizeof(FrameHeader));

    // Verify magic
    if (std::memcmp(header.magic, FRAME_MAGIC_BYTES, 2) != 0) {
        return false;
    }

    if (header.frag_count == 0 || header.frag_index >= header.frag_count) {
        return false;
    }

    size_t payloadLen = fragData.size() - sizeof(FrameHeader);
    if (header.frag_payload_len != payloadLen) {
        return false;
    }

    if (header.frag_index == 0 && header.block_header_offset != 0) {
        return false;
    }

    auto& file = files[header.file_uuid];
    auto& block = file.blocks[header.block_index];

    // Store fragment data (skipping frame header)
    std::vector<uint8_t> payload(fragData.begin() + sizeof(FrameHeader), fragData.end());

    bool headerPresent = false;
    uint16_t headerLen = 0;
    if (payload.size() >= sizeof(BlockHeader) &&
        std::memcmp(payload.data(), BLOCK_MAGIC_BYTES, 4) == 0) {
        BlockHeader bHeader;
        std::memcpy(&bHeader, payload.data(), sizeof(BlockHeader));
        if (bHeader.header_len >= sizeof(BlockHeader) && bHeader.header_len <= payload.size()) {
            headerPresent = true;
            headerLen = bHeader.header_len;

            if (block.headerBytes.empty()) {
                block.headerBytes.assign(payload.begin(), payload.begin() + headerLen);
            } else if (block.headerBytes.size() != headerLen ||
                       std::memcmp(block.headerBytes.data(), payload.data(), headerLen) != 0) {
                return false;
            }
        } else {
            return false;
        }
    }

    size_t dataOffset = headerPresent ? headerLen : 0;
    size_t dataLen = payload.size() - dataOffset;

    if (block.expectedFrags != 0 && block.expectedFrags != header.frag_count) {
        return false;
    }
    block.expectedFrags = header.frag_count;

    bool isNonLast = header.frag_index < header.frag_count - 1;
    bool canSetStandard = isNonLast && (!headerPresent || header.frag_index != 0);

    if (canSetStandard) {
        if (block.standardPayloadLen == 0) {
            block.standardPayloadLen = static_cast<uint16_t>(dataLen);
        } else if (dataLen != block.standardPayloadLen) {
            return false;
        }
    } else if (!isNonLast && block.standardPayloadLen != 0 && dataLen > block.standardPayloadLen) {
        return false;
    }

    auto existing = block.fragments.find(header.frag_index);
    if (existing != block.fragments.end() && existing->second.payloadLen != dataLen) {
        return false;
    }

    std::vector<uint8_t> dataPayload(payload.begin() + dataOffset, payload.end());
    block.fragments[header.frag_index] = {std::move(dataPayload),
                                          static_cast<uint16_t>(dataLen),
                                          header.block_header_offset};

    if (header.block_index > file.maxBlockIndex) {
        file.maxBlockIndex = header.block_index;
    }

    // Check if this is an EOF block (single-fragment block with EOF flag)
    if (header.frag_count == 1 && header.frag_index == 0 && !block.headerBytes.empty()) {
        const auto& headerData = block.headerBytes;
        if (headerData.size() >= sizeof(BlockHeader)) {
            BlockHeader bHeader;
            std::memcpy(&bHeader, headerData.data(), sizeof(BlockHeader));
            if (bHeader.flags & FLAG_EOF_BLOCK) {
                file.seenEof = true;
                const auto& fragPayload = block.fragments.at(0).payload;
                if (fragPayload.size() >= sizeof(uint32_t)) {
                    std::memcpy(&file.expectedDataBlocks,
                                fragPayload.data(), sizeof(uint32_t));
                }
            }
        }
    }

    return true;
}

bool Decoder::FileAssembly::isComplete() const {
    if (blocks.empty()) return false;

    // If we've seen EOF, use it to determine completeness
    if (seenEof && expectedDataBlocks > 0) {
        // Check all data blocks (0 to expectedDataBlocks-1) are complete
        for (uint32_t i = 0; i < expectedDataBlocks; ++i) {
            auto it = blocks.find(i);
            if (it == blocks.end() || !it->second.isComplete()) {
                return false;
            }
        }
        return true;
    }

    // Fallback: no EOF seen, use old heuristic
    for (uint32_t i = 0; i <= maxBlockIndex; ++i) {
        auto it = blocks.find(i);
        if (it == blocks.end() || !it->second.isComplete()) {
            return false;
        }
    }
    return true;
}

bool Decoder::isComplete(const UUID& uuid) const {
    auto it = files.find(uuid);
    if (it == files.end()) return false;
    return it->second.isComplete();
}

std::set<UUID> Decoder::getDetectedUuids() const {
    std::set<UUID> uuids;
    for (const auto& pair : files) {
        uuids.insert(pair.first);
    }
    return uuids;
}

void Decoder::finalize(const UUID& uuid, const std::string& outputPath) {
    auto it = files.find(uuid);
    if (it == files.end()) return;

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) throw std::runtime_error("Could not open output file");

    const auto& file = it->second;

    // Determine how many data blocks to process
    uint32_t numDataBlocks = file.seenEof && file.expectedDataBlocks > 0
                             ? file.expectedDataBlocks
                             : file.maxBlockIndex + 1;

    for (uint32_t b = 0; b < numDataBlocks; ++b) {
        if (file.blocks.find(b) == file.blocks.end()) {
             std::cerr << "Block " << b << " is completely missing." << std::endl;
             continue;
        }
        const auto& block = file.blocks.at(b);

        const auto& headerData = block.headerBytes;
        if (headerData.size() < sizeof(BlockHeader)) {
            std::cerr << "Block " << b << " missing header data." << std::endl;
            continue;
        }

        BlockHeader bHeader;
        std::memcpy(&bHeader, headerData.data(), sizeof(BlockHeader));

        if (bHeader.header_len < sizeof(BlockHeader) || bHeader.header_len > headerData.size()) {
            std::cerr << "Invalid header length in block " << b << std::endl;
            continue;
        }

        // Skip EOF blocks (shouldn't be in data range, but safety check)
        if (bHeader.flags & FLAG_EOF_BLOCK) continue;

        bool useFEC = (bHeader.flags & FLAG_OUTER_FEC);
        bool hasCRC = (bHeader.flags & FLAG_HAS_PAYLOAD_CRC);
        bool hasHeaderCRC = (bHeader.flags & FLAG_HAS_HEADER_CRC);
        bool isCompressed = (bHeader.flags & FLAG_COMPRESSED);
        uint8_t codecId = 0;
        uint16_t fecLen = 0;
        uint32_t originalLen = 0;
        uint32_t expectedCrc = 0;
        uint32_t expectedHeaderCrc = 0;
        size_t headerTotalLen = bHeader.header_len;

        if (hasHeaderCRC) {
            if (headerTotalLen < sizeof(BlockHeader) + sizeof(uint32_t)) {
                std::cerr << "Header CRC flagged but header is too small in block " << b << std::endl;
                continue;
            }
            std::memcpy(&expectedHeaderCrc,
                        headerData.data() + headerTotalLen - sizeof(uint32_t),
                        sizeof(uint32_t));
            size_t crcLen = headerTotalLen - sizeof(uint32_t);
            uint32_t actualHeaderCrc = CRC32_ProcessBuffer(CRC32_INITIAL, headerData.data(), crcLen);
            actualHeaderCrc ^= 0xFFFFFFFF;
            if (actualHeaderCrc != expectedHeaderCrc) {
                std::cerr << "Header CRC mismatch in block " << b
                          << " (expected " << std::hex << expectedHeaderCrc
                          << ", got " << actualHeaderCrc << std::dec << ")" << std::endl;
                continue;
            }
        }

        if (useFEC) {
            size_t extLen = headerTotalLen - sizeof(BlockHeader)
                            - (hasHeaderCRC ? sizeof(uint32_t) : 0);
            if (extLen < sizeof(FECHeaderExt)) {
                std::cerr << "FEC header too small in block " << b << std::endl;
                continue;
            }

            const uint8_t* ptr = headerData.data() + sizeof(BlockHeader);
            FECHeaderExt fecExt;
            std::memcpy(&fecExt, ptr, sizeof(FECHeaderExt));
            codecId = fecExt.codec_id;
            fecLen = fecExt.fec_len;
            originalLen = fecExt.original_len;
            expectedCrc = fecExt.payload_crc32;
        }

        // Reconstruct block data buffer
        // Logical layout across fragments (data portion only):
        // [Payload] [Parity]
        size_t totalDataSize = bHeader.payload_len + fecLen;
        std::vector<uint8_t> dataBlock(totalDataSize, 0);
        std::vector<int> erasures;
        
        // Fill known data
        // We need to know where each fragment goes in the dataBlock.
        // Decoder currently stores fragments by index.
        // We need to calculate offsets.
        // Assumption: Fragments are sequential and (mostly) fixed size.
        // Frag 0 starts at 0.
        // Frag 1 starts at Frag0.size().
        // ...
        // BUT if we are missing fragments, we don't know their exact sizes!
        // WEAKNESS: Variable fragment sizes (due to "last fragment" or compression) makes random access hard without a map.
        // However, Encoder uses fixed `fragmentSize` (target) for all except maybe last.
        // We can infer offset = frag_index * fragmentSize?
        // Not exactly, because FrameHeader is stripped.
        // The payload of Frag `i` corresponds to offset in `dataBlock`.
        // If Encoder uses fixed chunk size C for block payload splitting:
        // Offset = i * C.
        // How do we know C?
        // FrameHeader has `frag_payload_len`.
        // If we have at least one "middle" fragment, we might guess C.
        // Or if we assume Encoder defaults (800).
        // Better: FrameHeader should probably tell us the offset?
        // The current spec says: "block_header_offset" (used for first frag).
        // It does NOT have a "block_data_offset".
        // FIX: We assume standard fragment size based on the first fragment we find?
        // Or we rely on `frag0.size()` (minus header)?
        // Frag 0 has the header, so its payload contribution is `frag0.size()`.
        
        // Let's assume constant fragment capacity C (except last).
        // We can try to deduce C from any fragment.
        // If we have Frag 0, let C = frag0.size().
        // Wait, Frag 0 includes header. Subsequent frags don't.
        // Encoder logic:
        // Chunk 0: Size C. Contains Header + Data.
        // Chunk 1: Size C. Contains Data.
        // So offset of Chunk 1 is C.
        // The `fragData` stored in `block.fragments` DOES NOT include FrameHeader.
        // So it is purely chunks of the `blockData`.
        
        // Try to deduce a standard fragment size (all non-last fragments).
        size_t chunkSize = block.standardPayloadLen;
        if (chunkSize == 0) {
            for (const auto& pair : block.fragments) {
                chunkSize = std::max(chunkSize, static_cast<size_t>(pair.second.payloadLen));
            }
        }
        if (chunkSize == 0) {
            std::cerr << "Block " << b << " has no usable fragment sizes." << std::endl;
            continue;
        }

        size_t lastFragLen = chunkSize;
        if (block.expectedFrags > 0) {
            size_t base = chunkSize * (block.expectedFrags - 1);
            if (totalDataSize > base) {
                lastFragLen = totalDataSize - base;
                if (lastFragLen > chunkSize) {
                    std::cerr << "Block " << b << " has inconsistent fragment sizing." << std::endl;
                    lastFragLen = chunkSize;
                }
            }
        }
        
        for (uint16_t f = 0; f < block.expectedFrags; ++f) {
            size_t offset = f * chunkSize;
            
            size_t expectedLen = (f + 1 == block.expectedFrags) ? lastFragLen : chunkSize;

            if (block.fragments.count(f)) {
                const auto& frag = block.fragments.at(f);
                size_t fragLen = frag.payloadLen;
                if (fragLen > expectedLen) {
                    fragLen = expectedLen;
                }
                if (offset + fragLen <= dataBlock.size()) {
                    std::memcpy(dataBlock.data() + offset, frag.payload.data(), fragLen);
                } else if (offset < dataBlock.size()) {
                    std::memcpy(dataBlock.data() + offset, frag.payload.data(), dataBlock.size() - offset);
                }
            } else if (expectedLen > 0) {
                // Missing fragment -> Erasures
                // Mark bytes [offset, offset + expectedLen) as erased
                for (size_t k = 0; k < expectedLen; ++k) {
                    if (offset + k < dataBlock.size()) {
                        erasures.push_back(offset + k);
                    }
                }
            }
        }
        
        // Now we have `dataBlock` populated with what we have.
        // [Payload][Parity]
        if (useFEC && !erasures.empty()) {
            size_t fecZoneSize = bHeader.payload_len + fecLen;
            
            if (dataBlock.size() >= fecZoneSize) {
                std::vector<uint8_t> fecBuffer(fecZoneSize);
                std::memcpy(fecBuffer.data(), dataBlock.data(), fecZoneSize);

                std::vector<int> relativeErasures;
                for (int e : erasures) {
                    if (e >= 0 && e < (int)fecZoneSize) {
                        relativeErasures.push_back(e);
                    }
                }

                // Attempt decode
                if (fec.decode(fecBuffer, relativeErasures, FEC_INTERLEAVE_DEPTH)) {
                    std::cout << "Recovered block " << b << " using FEC!" << std::endl;
                    // Copy back
                    std::memcpy(dataBlock.data(), fecBuffer.data(), fecBuffer.size());
                } else {
                    std::cerr << "Failed to recover block " << b << " (too many errors)" << std::endl;
                }
            }
        }

        // Extract and decompress payload
        const uint8_t* rawPayload = dataBlock.data();
        std::vector<uint8_t> finalPayload;

        if (isCompressed && codecId == 1 && originalLen > 0) {
            // Decompress using zlib
            finalPayload.resize(originalLen);
            uLongf destLen = originalLen;
            int zret = uncompress(finalPayload.data(), &destLen,
                                  rawPayload, bHeader.payload_len);
            if (zret != Z_OK) {
                std::cerr << "Decompression failed for block " << b
                          << " (zlib error " << zret << ")" << std::endl;
                continue;
            }
            finalPayload.resize(destLen);
        } else {
            // No compression - use original data
            uint32_t payloadSize = (originalLen > 0) ? originalLen : bHeader.payload_len;
            finalPayload.assign(rawPayload, rawPayload + payloadSize);
        }

        // Verify CRC if present (on original uncompressed data)
        if (hasCRC) {
            uint32_t actualCrc = CRC32_ProcessBuffer(CRC32_INITIAL,
                                                      finalPayload.data(), finalPayload.size());
            actualCrc ^= 0xFFFFFFFF;
            if (actualCrc != expectedCrc) {
                std::cerr << "CRC mismatch in block " << b
                          << " (expected " << std::hex << expectedCrc
                          << ", got " << actualCrc << std::dec << ")" << std::endl;
            } else {
                std::cout << "Block " << b << " CRC verified." << std::endl;
            }
        }

        outFile.write(reinterpret_cast<const char*>(finalPayload.data()), finalPayload.size());
    }
}

bool Decoder::processImage(const std::string& imagePath) {
    std::ifstream file(imagePath, std::ios::binary);
    if (!file) return false;

    std::string magic;
    std::string token;
    int width = 0;
    int height = 0;
    int maxVal = 0;
    if (!readPpmToken(file, magic)) return false;
    try {
        if (!readPpmToken(file, token)) return false;
        width = std::stoi(token);
        if (!readPpmToken(file, token)) return false;
        height = std::stoi(token);
        if (!readPpmToken(file, token)) return false;
        maxVal = std::stoi(token);
    } catch (const std::exception&) {
        return false;
    }

    if (magic != "P6") {
        std::cerr << "Only PPM P6 is supported in prototype decoder." << std::endl;
        return false;
    }
    if (width <= 0 || height <= 0 || maxVal <= 0 || maxVal > 255) {
        return false;
    }

    std::vector<uint8_t> rgb(width * height * 3);
    file.read(reinterpret_cast<char*>(rgb.data()), rgb.size());

    std::vector<uint8_t> gray(width * height);
    for (int i = 0; i < width * height; ++i) {
        gray[i] = (uint8_t)(0.299 * rgb[i*3] + 0.587 * rgb[i*3+1] + 0.114 * rgb[i*3+2]);
    }

    zbar::ImageScanner scanner;
    scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
    scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_BINARY, 1);

    zbar::Image image(width, height, "Y800", gray.data(), width * height);
    int numSymbols = scanner.scan(image);

    for (zbar::Image::SymbolIterator symbol = image.symbol_begin();
         symbol != image.symbol_end(); ++symbol) {
        const uint8_t* raw = reinterpret_cast<const uint8_t*>(zbar_symbol_get_data(*symbol));
        size_t len = zbar_symbol_get_data_length(*symbol);
        std::vector<uint8_t> data(raw, raw + len);
        addFragment(data);
    }

    return numSymbols > 0;
}
