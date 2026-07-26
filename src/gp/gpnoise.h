#ifndef _GPNOISE_H_
#define _GPNOISE_H_

#include "kernel.h"
#include "memory"

namespace mupsi {

struct ConditioningState {
    bool active = false;
    Vector3f C = Vector3f::Zero();
    float u_tilde = 0.0f;
    Vector3f gradient_scale = Vector3f::Zero();
};

class GPNoiseGenerator {

public:

  GPNoiseGenerator(const SEKernel& kernel,  int impulse_density, uint32_t seed):
    kernel(kernel), impulseDensity(impulse_density), gpseed(seed) {
  }

  float RawNoise(const Vector3f& pos) const { return RawNoise(pos, gpseed); }
  float RawNoise(const Vector3f& pos, uint32_t seed) const;
  float Var() const {return ((impulseDensity / std::pow(kernel.getKernelRadius(), 3)) * std::pow(M_PI, 1.5)); }
  float Noise(const Vector3f& pos) const { return kernel.getSigma() * RawNoise(pos) / std::sqrt(Var()); }
  float Noise(const Vector3f& pos, uint32_t seed) const { return kernel.getSigma() * RawNoise(pos, seed) / std::sqrt(Var()); }

  const SEKernel& getKernel() const { return kernel; }
  uint32_t getSeed() const { return gpseed; }

private:

  SEKernel kernel;
  int impulseDensity;
  uint32_t gpseed;

};

class Sampler; 
class SparseConvKernel; 

extern float gpeps; 

class SparseGPNoiseGenerator {

  std::shared_ptr<SparseConvKernel> kernel_; 
  int impulseDensity_; 

  private: 
  
  float InternalNoise(const Vector3f& p, uint32_t seed) const; 

  public: 

  SparseGPNoiseGenerator(std::shared_ptr<SparseConvKernel> kernel, int impulse_density):
    kernel_(kernel), impulseDensity_(impulse_density){}

    // aware: need to setSeed before call it
  float RawNoise(const Vector3f& p, uint32_t seed) const {return kernel_->getSigma() * InternalNoise(p, seed) / std::sqrt(Var()); } 
  float Var() const {return ((impulseDensity_ / std::pow(kernel_->getKernelRadius(), 3)) * std::pow(M_PI, 1.5)); }

  Vector3f Gradient(const Vector3f& p, uint32_t seed) const;

  std::shared_ptr<SparseConvKernel> getKernel() const { return kernel_; }

}; 

}

#endif 