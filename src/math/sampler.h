#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include "random.h"
#include <Eigen/Core> 

using namespace Eigen; 

namespace mupsi {

class Sampler {

public:

    Sampler() = default;
    virtual ~Sampler() = default;
    virtual float next1D() = 0;
    virtual void next2D(float& x, float& y) = 0;
    Vector2f next2D() {
        float x, y;
        next2D(x, y);
        return Vector2f(x, y);
    }
    virtual uint32_t nextI() = 0;

};

class UniformPathSampler : public Sampler {
    Random rng_;

public:
    UniformPathSampler(uint32_t seed): rng_(seed) {}
    float next1D() override { return rng_.next1D(); }
    void next2D(float& x, float& y) override { x = rng_.next1D(); y = rng_.next1D(); }
    uint32_t nextI() override { return rng_.nextI(); }
};

class ConstantSampler: public Sampler {
    float seed_; 
    float x_, y_;
    uint32_t i_;  

public:
    ConstantSampler(float seed): seed_(seed) {
        Random rng(seed_);
        x_ = rng.next1D();
        y_ = rng.next1D();
        i_ = rng.nextI();
    }

    float next1D() override { return x_; }
    void next2D(float& x, float& y) override { x = x_; y = y_; }
    uint32_t nextI() override { return i_; }
};


}

#endif 