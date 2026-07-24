#ifndef _MEDIUM_H_
#define _MEDIUM_H_

#include <Eigen/Core>

using namespace Eigen;

namespace mupsi {

  struct GPConditioningState {
      bool active = false;
      Vector3f C = Vector3f::Zero();
      float u_tilde = 0.0f;
      Vector3f gradient_scale = Vector3f::Zero();
  };

  class Bsdf;

  struct MediumSample {
    float t;
    Vector3f p;
    Vector3f normal;

    const GPConditioningState& conditioning;
    const Bsdf* bsdf;
  };

  class Ray;
  class Sampler;

  class Medium {

    public:

    virtual void sampleDistance(Ray& ray, MediumSample& sample, Sampler& sampler) const = 0;
    virtual Vector3f transmittance(const Ray& ray, Sampler& sampler) const = 0;

  };


}

#endif 