#ifndef _EXTERNALGPMEDIUM_H_
#define _EXTERNALGPMEDIUM_H_

#include "medium.h"
#include "gp/meanfunction.h"
#include <memory>
#include <vector>

namespace mupsi {

class ExternalGPMedium : public Medium {
  std::shared_ptr<GridMeanFunction> mean_;
  std::shared_ptr<GridMeanFunction> variance_;  // null = deterministic
  float noiseScale_;
  std::shared_ptr<Bsdf> bsdf_;

  // deterministic noise from (position, seed) like GPMedium
  float hashNoise(const Vector3f& p, uint32_t seed) const;

  float eval(const Vector3f& p, uint32_t seed) const;
  float eval(const Vector3f& p, uint32_t seed, GPConditioningState& state) const;
  Vector4f evalWithGradient(const Vector3f& p, uint32_t seed) const;
  Vector4f evalWithGradient(const Vector3f& p, uint32_t seed, GPConditioningState& state) const;

public:
  ExternalGPMedium(std::shared_ptr<GridMeanFunction> mean,
                   std::shared_ptr<GridMeanFunction> variance,
                   float noiseScale = 1.0f)
    : mean_(mean), variance_(variance), noiseScale_(noiseScale), bsdf_(default_bsdf_) {}

  bool sampleDistance(Ray& ray, MediumSample& sample, Sampler& medium_sampler) const override;
  Vector3f transmittance(const Ray& ray, MediumSample& sample, Sampler& medium_sampler) const override;

  std::shared_ptr<Bsdf> getBsdf() const {
    return default_bsdf_;
  }
};

} // namespace mupsi

#endif
