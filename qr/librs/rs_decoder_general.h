#ifndef RS_DECODER_GENERAL_H
#define RS_DECODER_GENERAL_H

#include "rs_decoder_base.h"
#include <vector>
#include <array>

class RS_DECODER_GENERAL : public RS_DECODER_BASE {
public:
    explicit RS_DECODER_GENERAL(int tt, int b0);
    ~RS_DECODER_GENERAL() = default;

    int RSDecode(GF recd[nn]) override;
    int RSDecodeErasures(GF recd[nn], int eras_pos[2 * MAX_TT], int no_eras) override;

private:
    void calculate_syndromes(const GF recd[nn], std::vector<GF>& syndromes) override;
    // Precomputed power tables as class members
    std::vector<std::vector<GF>> precomp_powers;
    
    // Reusable buffers as class members to avoid allocations
    // These are kept sized to the actual tt_ value and reused across calls
    std::vector<GF> syndromes_buffer;
    std::vector<GF> lambda_buffer;
    std::vector<GF> root_buffer;
    std::vector<GF> loc_buffer;
    std::vector<GF> omega_buffer;
};

#endif // RS_DECODER_GENERAL_H
