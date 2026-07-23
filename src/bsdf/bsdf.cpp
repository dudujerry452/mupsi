#include "bsdf.h"
#include "math/random.h"
#include "math/sample.h"
#include <Eigen/Geometry>

namespace mupsi {

  void LambertianBsdf::eval(SurfaceScatterEvent& event) const {
    event.rad = albedo_ * event.wi.dot(event.normal) / M_PI;
  }

  void LambertianBsdf::sample(SurfaceScatterEvent& event) const {
    // sample wi using cosine-weighted hemisphere sampling
    float r1 = event.sampler->next1D();
    float r2 = event.sampler->next1D();
    float phi = 2 * M_PI * r1;
    float cosTheta = sqrt(1 - r2);
    float sinTheta = sqrt(r2);
    Vector3f localWi(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    TangentFrame frame(event.normal); 
    event.wi = frame.toGlobal(localWi);

    event.pdf = cosTheta / M_PI;
    event.rad = albedo_; 

  }

  void LambertianBsdf::pdf(SurfaceScatterEvent& event) const {
    float cosTheta = std::max(event.wi.dot(event.normal), 0.0f);
    event.pdf = cosTheta / M_PI;
  }

} 