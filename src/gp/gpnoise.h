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

  // 1D noise along ray: t = distance along ray in world units
  // returns {value, grad_x, grad_y, grad_z} in isotropic ray space (ray = Z)
  Vector4f InternalNoise1D(float t, uint32_t seed) const;
  float Var1D() const {return (impulseDensity_ / kernel_->getKernelRadius()) * std::sqrt(M_PI); }

  public:

  SparseGPNoiseGenerator(std::shared_ptr<SparseConvKernel> kernel, int impulse_density):
    kernel_(kernel), impulseDensity_(impulse_density){}

  // x: value, yzw: gradient
  Vector4f RawNoise(const Vector3f& p, uint32_t seed) const {return kernel_->getSigma() * InternalNoise(p, seed) / std::sqrt(Var()); }
  Vector4f RawNoise1D(float t, uint32_t seed) const {return kernel_->getSigma() * InternalNoise1D(t, seed) / std::sqrt(Var1D()); }

  std::shared_ptr<SparseConvKernel> getKernel() const { return kernel_; }

}; 

}

#endif 