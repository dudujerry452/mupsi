#ifndef _GPNOISE_H_
#define _GPNOISE_H_

#include "kernel.h"
#include "memory"

namespace mupsi {

struct GPSettings {

  float gpeps = 1e-4; 
  enum class GPCorrelationMode { SingleRealization, RenewalPlus };
  GPCorrelationMode gpMode = GPCorrelationMode::SingleRealization;

}; 

extern GPSettings g_gpSettings;

class Sampler; 
class SparseConvKernel; 


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