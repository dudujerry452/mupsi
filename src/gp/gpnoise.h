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
  
  Vector4f InternalNoise(const Vector3f& p, uint32_t seed) const; 
  float Var() const {return ((impulseDensity_ / std::pow(kernel_->getKernelRadius(), 3)) * std::pow(M_PI, 1.5)); }


  public: 

  SparseGPNoiseGenerator(std::shared_ptr<SparseConvKernel> kernel, int impulse_density):
    kernel_(kernel), impulseDensity_(impulse_density){}

  // x: value, yzw: gradient 
  Vector4f RawNoise(const Vector3f& p, uint32_t seed) const {return kernel_->getSigma() * InternalNoise(p, seed) / std::sqrt(Var()); } 

  std::shared_ptr<SparseConvKernel> getKernel() const { return kernel_; }

}; 

}

#endif 