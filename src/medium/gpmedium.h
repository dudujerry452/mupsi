#ifndef _GPMEDIUM_H_
#define _GPMEDIUM_H_

#include "medium.h"
#include "gp/gpnoise.h"
#include "gp/meanfunction.h"
#include <memory> 

using namespace Eigen; 

namespace mupsi {



extern GPSettings g_gpSettings;

class MeanFunction; 

class GPMedium : public Medium {

  std::shared_ptr<MeanFunction> mean_; 
  std::shared_ptr<SparseGPNoiseGenerator> noiseGenerator_;

  std::shared_ptr<Bsdf> bsdf_;

  private: 

  float evalMu(const Vector3f& p) const {return mean_->eval(p); }
  Vector3f muGradient(const Vector3f& p) const {return mean_->gradient(p); }
  Vector4f evalPsi(const Vector3f& p, uint32_t seed) const { return noiseGenerator_->RawNoise(p, seed); }


  float eval(const Vector3f& p, uint32_t seed) const; 
  float eval(const Vector3f& p, uint32_t seed, GPConditioningState& state) const;  
  Vector4f evalWithGradient(const Vector3f& p, uint32_t seed) const;
  Vector4f evalWithGradient(const Vector3f& p, uint32_t seed, GPConditioningState& state) const;

  public: 
  GPMedium(std::shared_ptr<MeanFunction> mean_func, std::shared_ptr<SparseGPNoiseGenerator> noiseGenerator) : mean_(mean_func), noiseGenerator_(noiseGenerator), bsdf_(default_bsdf_) {}
  GPMedium(std::shared_ptr<MeanFunction> mean_func, std::shared_ptr<SparseGPNoiseGenerator> noiseGenerator, std::shared_ptr<Bsdf> bsdf) : mean_(mean_func), noiseGenerator_(noiseGenerator), bsdf_(bsdf) {}

  // smapler: 
  bool sampleDistance(Ray& ray, MediumSample& sample, Sampler& medium_sampler) const override;
  Vector3f transmittance(const Ray& ray, MediumSample& state, Sampler& medium_sampler) const override; 

  std::shared_ptr<Bsdf> getBsdf() const {
    return default_bsdf_; 
  }

  void sampleCondition(MediumSample& sample, Sampler& medium_sampler) const;
};

}

#endif 