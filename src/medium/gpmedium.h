#ifndef _GPMEDIUM_H_
#define _GPMEDIUM_H_

#include "medium.h"
#include "gp/gpnoise.h"
#include <memory> 

using namespace Eigen; 

namespace mupsi {

class GPMedium : public Medium {

  std::shared_ptr<SparseGPNoiseGenerator> noiseGenerator_;

  public: 

  GPMedium(std::shared_ptr<SparseGPNoiseGenerator> noiseGenerator) : noiseGenerator_(noiseGenerator) {}

  void sampleDistance(Ray& ray, MediumSample& sample, Sampler& sampler) const override;
  Vector3f transmittance(const Ray& ray, Sampler& sampler) const override { return Vector3f::Zero(); }

};

}

#endif 