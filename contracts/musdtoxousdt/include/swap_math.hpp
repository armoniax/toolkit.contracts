#pragma once
#include <cstdint>

namespace swap_math {
// Asset limits keep each product within signed 128 bits.
inline __int128 gross(int64_t amount, int64_t numerator, int64_t denominator) {
    return static_cast<__int128>(amount) * numerator / denominator;
}
// Round the configured output-token fee upward so dust cannot bypass it.
inline int64_t fee(int64_t gross_amount, uint16_t fee_bps) {
    return static_cast<int64_t>((static_cast<__int128>(gross_amount) * fee_bps + 9999) / 10000);
}
}
