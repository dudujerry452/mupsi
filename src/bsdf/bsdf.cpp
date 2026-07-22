#include "bsdf.h"
#include "math/random.h"
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

    // transform localWi to world space
    Vector3f tangent, bitangent;
    if (fabs(event.normal.x()) > fabs(event.normal.z())) {
      tangent = Vector3f(-event.normal.y(), event.normal.x(), 0).normalized();
    } else {
      tangent = Vector3f(0, -event.normal.z(), event.normal.y()).normalized();
    }
    bitangent = event.normal.cross(tangent);
    event.wi = (tangent * localWi.x() + bitangent * localWi.y() + event.normal * localWi.z()).normalized();

  }

  void LambertianBsdf::pdf(SurfaceScatterEvent& event) const {
    float cosTheta = std::max(event.wo.dot(event.normal), 0.0f);
    event.pdf = (cosTheta > 0 ? 1.0f : 0.0f) / M_PI;
  }

} 