#ifndef ENCODER_H
#define ENCODER_H

#include <string>
#include <vector>
#include <functional>
#include "qrtape_defs.h"
#include "RS.h"

class Encoder {
public:
    Encoder(const std::string& inputPath);
    
    // Set parameters
    void setBlockSize(size_t size) { blockSize = size; }
    void setFragmentSize(size_t size) { fragmentSize = size; }

    // Process the file. For each fragment generated, call the callback with the raw bytes.
    void process(std::function<void(const std::vector<uint8_t>&, size_t blockIdx, size_t fragIdx)> callback);

private:
    std::string inputPath;
    UUID fileUuid;
    size_t blockSize = 4096;      // Default block payload size
    size_t fragmentSize = 800;    // Default target payload per QR (conservative)
    InterleavedRS fec;            // Interleaved RS encoder

    void generateUuid();
    void createBlock(uint32_t blockIndex, const std::vector<uint8_t>& payload,
                     std::function<void(const std::vector<uint8_t>&, size_t, size_t)> callback);
    void createEofBlock(uint32_t blockIndex, uint32_t totalDataBlocks,
                        std::function<void(const std::vector<uint8_t>&, size_t, size_t)> callback);
};

#endif // ENCODER_H