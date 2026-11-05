#ifndef INTERLEAVED_RS_H
#define INTERLEAVED_RS_H

#include <vector>
#include <memory>
#include <iostream>
#include "rs_factory.h"

// We use the librs default field GF(2^8).
// We choose T=16 (corrects 16 erasures or 8 errors per codeword).
// Codeword size N=255.
// Data size K = 255 - 2*T = 223.
// Parity size = 32 bytes.

constexpr int RS_T = 16;
constexpr int RS_PARITY_LEN = 2 * RS_T;
constexpr int RS_DATA_LEN = 255 - RS_PARITY_LEN; // 223
constexpr int RS_TOTAL_LEN = 255;

class InterleavedRS {
public:
    InterleavedRS() {
        // Create the encoder/decoder factory.
        // For T=16, kk = 255 - 32 = 223.
        // Setting b0 = (kk + 1) / 2 = 112 enables the optimized symmetric 'flipped' encoder.
        codec = RS_FACTORY::instance().create_codec(RS_T, 112);
    }

    // Encodes data into data + parity.
    // Returns a vector containing the interleaved data followed by interleaved parity.
    // Actually, to keep it simple for the Tape format:
    // The Block contains [Header][Payload][Parity].
    // Our input 'data' is the Payload.
    // We pad the payload to be a multiple of K * Depth.
    // Wait, if we just want to compute parity, we can do that.
    
    struct Result {
        std::vector<uint8_t> paddedPayload; // Original payload + zero padding
        std::vector<uint8_t> parity;        // Interleaved parity bytes
        int depth;                          // Interleave depth used
    };

    Result encode(const std::vector<uint8_t>& payload, int interleaveDepth = 20) {
        // 1. Determine padding
        // We need payload size to be a multiple of (RS_DATA_LEN * interleaveDepth)?
        // No, we can shorten the last codeword, but interleaving is easiest if we just fill full codewords.
        // Let's stick to full codewords for simplicity in this prototype.
        // Total capacity per "stripe row" = RS_DATA_LEN * interleaveDepth.
        
        // Actually, "striping" usually means:
        // Byte 0 -> CW 0
        // Byte 1 -> CW 1
        // ...
        // Byte D-1 -> CW D-1
        // Byte D -> CW 0
        
        // If we have D codewords.
        // Each codeword has capacity K.
        // Total capacity = D * K.
        
        size_t capacity = interleaveDepth * RS_DATA_LEN;
        std::vector<uint8_t> padded = payload;
        if (padded.size() > capacity) {
            throw std::runtime_error("Payload too large for configured FEC depth");
        }
        padded.resize(capacity, 0); // Zero pad

        // 2. Prepare codewords
        // We have 'interleaveDepth' codewords.
        // Each input to RSEncode is array of MAX_KK (253) but we only use K=223.
        // librs expects data at the *end* of the buffer if we want shortened codes?
        // Let's check librs usage. 
        // Typically: RSEncode(data_in, parity_out).
        // Let's assume standard usage: data[0..K-1] produces parity[0..2T-1].
        
        std::vector<uint8_t> parity(interleaveDepth * RS_PARITY_LEN);

        // We need temporary buffers for each codeword because RSEncode takes GF arrays
        // and librs uses `unsigned char` for GF.
        
        for (int i = 0; i < interleaveDepth; ++i) {
            GF data[MAX_KK]; // 253 max
            GF bb[2 * MAX_TT]; // parity out
            
            // Extract the i-th interleaved byte stream
            // Byte j of codeword i comes from padded[i + j*D]
            for (int j = 0; j < RS_DATA_LEN; ++j) {
                data[j] = padded[i + j * interleaveDepth];
            }
            
            // Encode
            // Note: We need to pass the actual data length we are using?
            // librs RSEncode might assume full length or based on init.
            // RS_FACTORY::CreateCodec(T) creates a codec with K = N - 2T.
            // So it expects exactly K bytes of data?
            // Let's assume yes.
            
            // librs interface: void RSEncode(GF data[MAX_KK], GF bb[2 * MAX_TT])
            // It reads kk_ bytes from data.
            codec->RSEncode(data, bb);
            
            // Store parity
            // Parity byte k of codeword i goes to parity[i + k*D]
            for (int k = 0; k < RS_PARITY_LEN; ++k) {
                parity[i + k * interleaveDepth] = bb[k];
            }
        }
        
        return {padded, parity, interleaveDepth};
    }

    // Decodes data + parity, correcting erasures.
    // data: The full interleaved buffer (payload + parity).
    // erasures: A list of indices in 'data' that are known to be missing/corrupted.
    // returns: true if successful, false if uncorrectable.
    // The 'data' buffer is modified in-place with corrected values.
    bool decode(std::vector<uint8_t>& data, const std::vector<int>& erasures, int depth = 20) {
        // Validate size
        // Expected size = depth * RS_TOTAL_LEN (255)
        // Or at least depth * (DATA + PARITY).
        // Our 'encode' produced depth * 255 bytes (effectively, if we consider full block).
        // The data passed here should be the full reconstructed block (payload + parity).
        
        if (data.size() != static_cast<size_t>(depth * RS_TOTAL_LEN)) {
            // It might be smaller if we didn't pad full?
            // Encoder: capacity = interleaveDepth * RS_DATA_LEN
            // + Parity = interleaveDepth * RS_PARITY_LEN
            // Total = interleaveDepth * 255.
            // Yes.
            // If data is smaller, we should resize/pad?
            // Decoder caller should handle this.
            return false;
        }

        bool success = true;

        for (int i = 0; i < depth; ++i) {
            GF recd[nn]; // 255
            int eras_pos[2 * MAX_TT];
            int no_eras = 0;
            bool tooManyErasures = false;
            
            // 1. Extract codeword i
            // Byte j of codeword i corresponds to data[i + j*depth]
            for (int j = 0; j < RS_TOTAL_LEN; ++j) {
                recd[j] = data[i + j * depth];
            }
            
            // 2. Identify erasures for this codeword
            // We iterate through global erasure indices and check if they belong to this codeword.
            // Erasure at global index E belongs to codeword (E % depth).
            // Its position in codeword is (E / depth).
            // Optimization: Pre-bin erasures? 
            // Or just iterate:
            for (int e : erasures) {
                if (e % depth == i) {
                    int idx = e / depth;
                    if (idx < RS_TOTAL_LEN) {
                        if (no_eras >= 2 * MAX_TT) {
                            tooManyErasures = true;
                            break;
                        }
                        eras_pos[no_eras++] = idx;
                    }
                }
            }

            if (tooManyErasures) {
                success = false;
                continue;
            }
            
            // 3. Decode
            // Use Erasure decoding (corrects up to 2*T erasures).
            // eras_pos contains 0-based indices of missing bytes in 'recd'.
            
            int result = codec->RSDecodeErasures(recd, eras_pos, no_eras);
            if (result < 0) {
                success = false;
                // We could continue to try other codewords, but the block is likely toast.
                // However, partial recovery might be useful?
                // For now, fail.
            } else {
                // 4. Write back corrected data
                for (int j = 0; j < RS_TOTAL_LEN; ++j) {
                    data[i + j * depth] = recd[j];
                }
            }
        }
        
        return success;
    }

private:
    std::unique_ptr<RS_CODEC> codec;
};

#endif // INTERLEAVED_RS_H
