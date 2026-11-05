#ifndef RS_FLIPPED_ENCODER_GENERAL_H
#define RS_FLIPPED_ENCODER_GENERAL_H

#include "rs_encoder_base.h"
#include <cstdint>
#include <cstring>
#include <vector>

/**
 * RS_FLIPPED_ENCODER_GENERAL - CRC table-driven Reed-Solomon encoder with ascending feedback processing
 *
 * This "flipped" encoder uses a lookup table approach similar to CRC algorithms and processes
 * feedback bytes in ascending order (from low to high indices), unlike the standard implementation
 * which processes in descending order (high to low indices).
 *
 * IMPORTANT: The flipped encoder implementation ONLY works when the generator polynomial
 * is symmetrical, which occurs when b0 = (kk + 1) / 2.
 *
 * A generator polynomial is symmetrical when g(x) = x^(2t) * g(1/x), meaning the coefficients
 * are mirrored around the middle term. This symmetry property allows the flipped direction
 * to produce identical parity bytes as the standard direction.
 *
 * The flipped implementation can be more efficient on modern processors due to better
 * memory access patterns and improved ability to use SIMD/vector operations.
 */

class RS_FLIPPED_ENCODER_GENERAL : public RS_ENCODER_BASE {
public:
    RS_FLIPPED_ENCODER_GENERAL(int tt, int b0);
    ~RS_FLIPPED_ENCODER_GENERAL();

    void RSEncode(GF data[MAX_KK], GF bb[2 * MAX_TT]) override;

private:
    void RSGenPoly();
    void RSGenTable();

    const GF* pow2poly_;
    const GF* poly2pow_;
    std::vector<GF> gg;     // Generator polynomial coefficients
    GF* ptable;             // Lookup table for encoding

    // Helpers for block operations - using memcpy to avoid undefined behavior
    static inline void process_block8(GF* bb, const GF* TableRow) {
        uint64_t next_block, table_block, out_block;
        std::memcpy(&next_block, bb + 1, 8);
        std::memcpy(&table_block, TableRow, 8);
        out_block = next_block ^ table_block;
        std::memcpy(bb, &out_block, 8);
    }

    static inline void process_block4(GF* bb, const GF* TableRow) {
        uint32_t next_block, table_block, out_block;
        std::memcpy(&next_block, bb + 1, 4);
        std::memcpy(&table_block, TableRow, 4);
        out_block = next_block ^ table_block;
        std::memcpy(bb, &out_block, 4);
    }

    // Process a single byte in the flipped encoder
    // Note the forward-looking index: bb[pos+1]
    // This is opposite to the standard encoder which looks backward: bb[pos-1]
    static inline void process_byte1(GF* bb, const GF* TableRow, int pos) {
        bb[pos] = bb[pos + 1] ^ TableRow[pos];
    }
};

#endif // RS_FLIPPED_ENCODER_GENERAL_H
