#ifndef RS_STANDARD_ENCODER_GENERAL_H
#define RS_STANDARD_ENCODER_GENERAL_H

#include "rs_encoder_base.h"
#include <cstdint>
#include <cstring>
#include <vector>

/**
 * RS_STANDARD_ENCODER_GENERAL - Standard Reed-Solomon encoder with descending feedback processing
 *
 * This is the traditional Reed-Solomon encoder implementation that processes feedback bytes
 * in descending order (from high to low indices). Unlike the flipped encoder implementation,
 * this encoder works with ANY valid generator polynomial, regardless of symmetry.
 *
 * The standard encoder is more general but may be less efficient on modern processors
 * compared to the flipped implementation because of memory access patterns and
 * reduced ability to optimize with SIMD/vector operations.
 *
 * This implementation should be used when b0 != (kk + 1) / 2, indicating
 * a non-symmetrical generator polynomial.
 */

class RS_STANDARD_ENCODER_GENERAL : public RS_ENCODER_BASE {
public:
    RS_STANDARD_ENCODER_GENERAL(int tt, int b0);
    ~RS_STANDARD_ENCODER_GENERAL();

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
        std::memcpy(&next_block, bb - 1, 8);
        std::memcpy(&table_block, TableRow, 8);
        out_block = next_block ^ table_block;
        std::memcpy(bb, &out_block, 8);
    }

    static inline void process_block4(GF* bb, const GF* TableRow) {
        uint32_t next_block, table_block, out_block;
        std::memcpy(&next_block, bb - 1, 4);
        std::memcpy(&table_block, TableRow, 4);
        out_block = next_block ^ table_block;
        std::memcpy(bb, &out_block, 4);
    }

    // Process a single byte in the standard encoder
    // Note the backward-looking index: bb[pos-1]
    // This is opposite to the flipped encoder which looks forward: bb[pos+1]
    static inline void process_byte1(GF* bb, const GF* TableRow, int pos) {
        bb[pos] = bb[pos - 1] ^ TableRow[pos];
    }
};

#endif // RS_STANDARD_ENCODER_GENERAL_H
