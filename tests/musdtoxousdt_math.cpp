#include "swap_math.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using swap_math::gross;
    using swap_math::fee;
    assert(gross(100000000, 1000000, 1000000) == 100000000);
    assert(fee(100000000, 30) == 300000);
    assert(gross(1000000, 2000000, 1000000) == 2000000);
    assert(gross(1000000, 1000000, 2000000) == 500000);
    assert(gross(1000000, 1, 3) == 333333);
    assert(fee(1, 30) == 1);
    assert(fee(334, 30) == 2);
    assert(fee(1000000, 0) == 0);
    assert(fee(1000000, 100) == 10000);
    assert(fee(10000, 9999) == 9999);
    const int64_t max_asset = (int64_t(1) << 62) - 1;
    assert(gross(max_asset, max_asset, max_asset) == max_asset);
    assert(gross(max_asset, max_asset, 1) > max_asset);
    assert(fee(max_asset, 30) > 0 && fee(max_asset, 30) < max_asset);
    for (int64_t n = 1; n < 100000; ++n) {
        assert(fee(n, 30) * 10000 >= n * 30);
        assert((fee(n, 30) - 1) * 10000 < n * 30);
    }
    std::cout << "PASS: exact ratios, reciprocal ratios, rounding, asset bounds, fee rounding\n";
}
