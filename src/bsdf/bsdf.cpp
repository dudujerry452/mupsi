#include "bsdf.h"
#include "math/sampler.h"
#include "math/sample.h"
#include "texture/texture.h"
#include <Eigen/Geometry>
#include <memory>

namespace mupsi {

  std::shared_ptr<Texture> Bsdf::default_albedo_ = std::make_shared<ConstantTexture>(Vector3f(0.8, 0.8, 0.8));

  void LambertianBsdf::eval(SurfaceScatterEvent& event) const {
    event.weight = (*albedo_)[event.info->uv] * event.wi.dot(event.normal) / M_PI;
  }

  void LambertianBsdf::sample(SurfaceScatterEvent& event) const {
    // sample wi using cosine-weighted hemisphere sampling
    float r1, r2; 
    event.sampler->next2D(r1, r2);
    float phi = 2 * M_PI * r1;
    float cosTheta = sqrt(1 - r2);
    float sinTheta = sqrt(r2);
    Vector3f localWi(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    TangentFrame frame(event.normal); 
    event.wi = frame.toGlobal(localWi);

    event.pdf = cosTheta / M_PI;
    event.weight = (*albedo_)[event.info->uv]; // eval / pdf 
  }

  void LambertianBsdf::pdf(SurfaceScatterEvent& event) const {
    float cosTheta = std::max(event.wi.dot(event.normal), 0.0f);
    event.pdf = cosTheta / M_PI;
  }

} 