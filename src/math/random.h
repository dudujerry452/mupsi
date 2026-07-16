#ifndef _RANDOM_H_
#define _RANDOM_H_ 

#include <cstdint> 
#include <tuple>
#include <random>


inline void hash_combine(uint32_t& seed, uint32_t v) {
  seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2); 
}

template<typename... Args> 
uint32_t make_seed(Args... args) {
  auto tup = std::make_tuple(static_cast<uint32_t>(args)...); 
  uint32_t h = 0; 
  std::apply([&](auto... vals){
    (hash_combine(h, vals), ...); 
  }, tup); 
  return h; 
}

class Random {
      mutable std::mt19937 engine_;     // mutable 因为分布是
  

  public:
      explicit Random(uint32_t seed) : engine_(seed) {}

      float uniform() const {          // [0, 1)
          static std::uniform_real_distribution<float> dist(0.0f,1.0f);
          return dist(engine_);
      }

      float standard_normal() const {  // N(0, 1)
          static std::normal_distribution<float> dist(0.0f, 1.0f);
          return dist(engine_);
      }
};





#endif 