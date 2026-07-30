#ifndef _SDFMEDIUM_H_
#define _SDFMEDIUM_H_

#include "medium.h"
#include "gp/meanfunction.h"
#include <memory>

using namespace Eigen;

namespace mupsi {

class MeanFunction;

class SDFMedium : public Medium {

  std::shared_ptr<MeanFunction> mean_;
  std::shared_ptr<Bsdf> bsdf_;

private:

  float eval(const Vector3f& p) const { return mean_->eval(p); }
  Vector4f evalWithGradient(const Vector3f& p) const;

public:
  SDFMedium(std::shared_ptr<MeanFunction> mean_func)
      : mean_(mean_func), bsdf_(default_bsdf_) {}
  SDFMedium(std::shared_ptr<MeanFunction> mean_func, std::shared_ptr<Bsdf> bsdf)
      : mean_(mean_func), bsdf_(bsdf) {}

  bool sampleDistance(Ray& ray, MediumSample& sample, Sampler& medium_sampler) const override;
  Vector3f transmittance(const Ray& ray, MediumSample& sample, Sampler& medium_sampler) const override;

  std::shared_ptr<Bsdf> getBsdf() const {
    return default_bsdf_;
  }
};

}

#endif
