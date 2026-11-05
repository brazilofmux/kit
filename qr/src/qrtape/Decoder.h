#ifndef DECODER_H
#define DECODER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include "qrtape_defs.h"
#include "RS.h"

class Decoder {
public:
    Decoder();

    // Adds raw fragment data (from a decoded QR code)
    bool addFragment(const std::vector<uint8_t>& fragData);

    // Processes an image file using zbar to extract fragments
    bool processImage(const std::string& imagePath);

    // Checks if the reassembly is complete (all blocks present)
    // Note: In this version, we assume the first fragment count we see is the truth.
    bool isComplete(const UUID& uuid) const;

    // Finalizes and writes the reassembled file
    void finalize(const UUID& uuid, const std::string& outputPath);

    // Helper to get all seen UUIDs
    std::set<UUID> getDetectedUuids() const;

private:
    struct FragmentData {
        std::vector<uint8_t> payload;
        uint16_t payloadLen = 0;
        uint16_t headerOffset = 0;
    };

    struct BlockAssembly {
        uint16_t expectedFrags = 0;
        uint16_t standardPayloadLen = 0;
        std::vector<uint8_t> headerBytes;
        std::map<uint16_t, FragmentData> fragments;
        bool isComplete() const {
            return expectedFrags > 0 && fragments.size() == expectedFrags;
        }
    };

    struct FileAssembly {
        std::map<uint32_t, BlockAssembly> blocks;
        uint32_t maxBlockIndex = 0;
        bool seenEof = false;
        uint32_t expectedDataBlocks = 0;  // From EOF block

        bool isComplete() const;
    };

    std::map<UUID, FileAssembly> files;
    InterleavedRS fec;
};

#endif // DECODER_H
