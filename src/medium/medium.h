#ifndef _MEDIUM_H_
#define _MEDIUM_H_

#include <Eigen/Core>
#include "bsdf/bsdf.h"
#include "geometry/ray.h"

using namespace Eigen;

namespace mupsi {

  struct GPConditioningState {
      bool active = false;
      Vector3f C = Vector3f::Zero();
      float u_tilde = 0.0f;
      Vector3f g_tilde = Vector3f::Zero();
  };

  class Bsdf;

  struct MediumSample {
    float t;
    Vector3f p;
    Vector3f normal;

    GPConditioningState conditioning;
    const Bsdf* bsdf;
  };

  class Ray;
  class Sampler;

  class Medium {

    protected: 

    static std::shared_ptr<Bsdf> default_bsdf_;

    public:

    virtual bool sampleDistance(Ray& ray, MediumSample& sample, Sampler& sampler) const = 0;
    virtual Vector3f transmittance(const Ray& ray, Sampler& sampler) const = 0;
    virtual Vector3f sampleGradient(const Vector3f& p, Sampler& sampler) const = 0; 

    void makeSurfaceEventFromMedium(const MediumSample& sample, Ray& ray, Sampler& sampler, SurfaceScatterEvent& event) const {
      event.wo = ray.direction(); 
      event.normal = sample.normal; 
      event.sampler = &sampler;

     

    }

    virtual std::shared_ptr<Bsdf> getBsdf() const = 0;
  };


}

#endif 