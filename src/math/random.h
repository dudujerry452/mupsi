#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <cstdint>
#include <cmath>

namespace mupsi {

// ——— xxhash32 ——— (same as sparse-gpis)
constexpr uint32_t PRIME32_2 = 2246822519U;
constexpr uint32_t PRIME32_3 = 3266489917U;
constexpr uint32_t PRIME32_4 = 668265263U;
constexpr uint32_t PRIME32_5 = 374761393U;

inline uint32_t xxhash32(uint32_t x) {
    uint32_t h = x + PRIME32_5;
    h = PRIME32_4 * ((h << 17) | (h >> 15));
    h = PRIME32_2 * (h ^ (h >> 15));
    h = PRIME32_3 * (h ^ (h >> 13));
    return h ^ (h >> 16);
}

inline uint32_t xxhash32(uint32_t x, uint32_t y) {
    uint32_t h = y + PRIME32_5 + x * PRIME32_3;
    h = PRIME32_4 * ((h << 17) | (h >> 15));
    h = PRIME32_2 * (h ^ (h >> 15));
    h = PRIME32_3 * (h ^ (h >> 13));
    return h ^ (h >> 16);
}

inline uint32_t xxhash32(uint32_t x, uint32_t y, uint32_t z) {
    uint32_t h = z + PRIME32_5 + x * PRIME32_3;
    h = PRIME32_4 * ((h << 17) | (h >> 15));
    h += y * PRIME32_3;
    h = PRIME32_4 * ((h << 17) | (h >> 15));
    h = PRIME32_2 * (h ^ (h >> 15));
    h = PRIME32_3 * (h ^ (h >> 13));
    return h ^ (h >> 16);
}

inline uint32_t xxhash32(uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
    uint32_t h = w + PRIME32_5 + x * PRIME32_3;
    h = PRIME32_4 * ((h << 17) | (h >> 15));
    h += y * PRIME32_3;
    h = PRIME32_4 * ((h << 17) | (h >> 15));
    h += z * PRIME32_3;
    h = PRIME32_4 * ((h << 17) | (h >> 15));
    h = PRIME32_2 * (h ^ (h >> 15));
    h = PRIME32_3 * (h ^ (h >> 13));
    return h ^ (h >> 16);
}

// ——— PCG random number generator ——— (same as sparse-gpis)
class Random {
    uint64_t state_;

public:
    explicit Random(uint64_t seed) : state_(seed) {
        if (state_ == 0) state_ = 1;
        nextI(); nextI(); // warmup
    }

    // unsigned int → float [0, 1), with normalize (same as sparse-gpis BitManip::normalizedUint)
    static float uintBitsToFloat(uint32_t i) {
        union { float f; uint32_t i; } u;
        u.i = i;
        return u.f;
    }

    static float normalizedUint(uint32_t i) {
        return uintBitsToFloat((i >> 9u) | 0x3F800000u) - 1.0f;
    }

    uint32_t nextI() {
        uint64_t oldState = state_;
        state_ = oldState * 6364136223846793005ULL + 1;
        uint32_t xorShifted = uint32_t(((oldState >> 18u) ^ oldState) >> 27u);
        uint32_t rot = oldState >> 59u;
        return (xorShifted >> rot) | (xorShifted << (uint32_t(-int32_t(rot)) & 31));
    }

    float next1D() { return normalizedUint(nextI()); }

    // Bernoulli distribution: ±1 equally likely (same as sparse-gpis)
    static float Bernoulli(float rv, float a, float b, float p) {
        return rv < p ? a : b;
    }

    float sign() { return Bernoulli(next1D(), -1.0f, 1.0f, 0.5f); }

    // Box-Muller: N(0,1) (caches spare for even/odd calls)
    float standard_normal() {
        float u1 = next1D();
        float u2 = next1D();
        float r  = std::sqrt(-2.0f * std::log(u1));
        float a  = 2.0f * float(M_PI) * u2;
        // use cosine only, discard sine (simpler but wastes 1 normal per call)
        return r * std::cos(a);
    }
};

} // namespace mupsi

#endif
