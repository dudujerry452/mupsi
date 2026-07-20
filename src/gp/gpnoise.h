#ifndef _GPNOISE_H_
#define _GPNOISE_H_

#include "kernel.h"

namespace mupsi {

class GPNoiseGenerator {

public: 

  GPNoiseGenerator(const SEKernel& kernel,  int impulse_density, uint32_t seed):  
    kernel(kernel), impulseDensity(impulse_density), gpseed(seed) {
  }

  float RawNoise(const Vector3f& pos) const; 
  float Var() const {return ((impulseDensity / std::pow(kernel.getKernelRadius(), 3)) * std::pow(M_PI, 1.5)); } 
  float Noise(const Vector3f& pos) const { return kernel.getSigma() * RawNoise(pos) / std::sqrt(Var()); }

  // RawNoise / Var -> psi(p)

private: 

  SEKernel kernel;
  int impulseDensity; 
  uint32_t gpseed;

};

}

#endif 