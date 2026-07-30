#include "sdfmedium.h"
#include "geometry/ray.h"
#include "gp/meanfunction.h"
#include "math/sampler.h"

#include <algorithm>

namespace mupsi {

  Vector4f SDFMedium::evalWithGradient(const Vector3f& p) const {
    float mean = mean_->eval(p);
    Vector3f mean_grad = mean_->gradient(p);
    return Vector4f(mean, mean_grad.x(), mean_grad.y(), mean_grad.z());
  }

  bool SDFMedium::sampleDistance(Ray& ray, MediumSample& sample, Sampler& medium_sampler) const
  {
    float nt = ray.nearT(), ft = ray.farT();
    ft = std::min(ft, 2000.0f);
    (void)medium_sampler;

    const float minStep = 1e-3f;
    const int   maxIter = 512;

    float t = nt;
    float f_c = eval(ray.origin());
    int iter = 0;

    while (t < ft && iter < maxIter) {
      Vector3f p = ray.origin() + ray.direction() * t;
      float f_prev = f_c;
      float t_prev = t;
      f_c = eval(p);

      // sign change → surface crossed, refine with bisection
      if (f_prev * f_c < 0.0f) {
        float lo = t_prev, hi = t;
        float f_lo = f_prev;
        for (int b = 0; b < 16; b++) {
          float mid = (lo + hi) * 0.5f;
          float f_mid = eval(ray.origin() + ray.direction() * mid);
          if (f_lo * f_mid <= 0.0f) {
            hi = mid;
          } else {
            lo = mid;
            f_lo = f_mid;
          }
        }

        sample.exited = false;
        sample.t = hi;
        sample.p = ray.origin() + ray.direction() * hi;
        Vector4f f = evalWithGradient(sample.p);
        sample.normal = f.tail<3>().normalized();
        sample.bsdf = bsdf_.get();
        return true;
      }

      // sphere tracing: |f_c| = safe distance to nearest surface
      float step = std::max(std::abs(f_c), minStep);
      t += step;
      iter++;
    }

    sample.exited = true;
    return false;
  }

  Vector3f SDFMedium::transmittance(const Ray& ray, MediumSample& sample, Sampler& medium_sampler) const {
    (void)sample;
    (void)medium_sampler;

    float nt = ray.nearT(), ft = ray.farT();
    ft = std::min(ft, 2000.0f);

    const float minStep = 1e-3f;
    const int   maxIter = 512;

    float t = nt;
    float f_c = eval(ray.origin());
    int iter = 0;

    while (t < ft && iter < maxIter) {
      Vector3f p = ray.origin() + ray.direction() * t;
      float f_prev = f_c;
      f_c = eval(p);

      if (f_prev * f_c < 0.0f) {
        return Vector3f::Zero(); // opaque
      }

      float step = std::max(std::abs(f_c), minStep);
      t += step;
      iter++;
    }

    return Vector3f::Ones(); // transparent
  }

}
