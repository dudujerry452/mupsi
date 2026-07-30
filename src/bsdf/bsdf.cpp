#include "bsdf.h"
#include "medium/medium.h"
#include "math/sampler.h"
#include "math/sample.h"
#include "texture/texture.h"
#include <Eigen/Geometry>
#include <memory>

namespace mupsi {

  std::shared_ptr<Texture> Bsdf::default_albedo_ = std::make_shared<ConstantTexture>(Vector3f(0.8, 0.8, 0.8));
  std::shared_ptr<Bsdf> Medium::default_bsdf_ = std::make_shared<LambertianBsdf>(); // after albedo_

  Vector3f LambertianBsdf::eval(SurfaceScatterEvent& event) const {
    return (*albedo_)[event.info->uv] * event.wi.dot(event.normal) / M_PI;
  }

  float LambertianBsdf::pdf(SurfaceScatterEvent& event) const {
    float cosTheta = std::max(event.wi.dot(event.normal), 0.0f);
    return cosTheta / M_PI;
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

  Vector3f NullBsdf::eval(SurfaceScatterEvent& event) const {
    if(event.wi.dot(event.wo) > 0.999f) return Vector3f(1.0f, 1.0f, 1.0f);
    else return Vector3f(0.0f, 0.0f, 0.0f);
  }
  float NullBsdf::pdf(SurfaceScatterEvent& event) const {
    if(event.wi.dot(-event.wo) > 0.999f) return 1.0f;
    else return 0.0f;
  }
  Vector3f NullBsdf::weight(SurfaceScatterEvent& event) const {
    if(event.wi.dot(-event.wo) > 0.999f) return Vector3f(1.0f, 1.0f, 1.0f);
    else return Vector3f(0.0f, 0.0f, 0.0f);
  }
  void NullBsdf::sample(SurfaceScatterEvent& event) const {
    event.wi = -event.wo; // just reflect back
    event.pdf = 1.0f;
    event.weight = Vector3f(1.0f, 1.0f, 1.0f);
  }

  Vector3f SpecularBsdf::eval(SurfaceScatterEvent& event) const {
    Vector3f wr = 2 * (event.wi.dot(event.normal)) * event.normal - event.wi;
    if(wr.dot(event.wo) > 0.999f) return Vector3f(1.0f, 1.0f, 1.0f);
    else return Vector3f(0.0f, 0.0f, 0.0f);
  }

  float SpecularBsdf::pdf(SurfaceScatterEvent& event) const {
    Vector3f wr = 2 * (event.wi.dot(event.normal)) * event.normal - event.wi;
    if(wr.dot(event.wo) > 0.999f) return 1.0f;
    else return 0.0f;
  }

  Vector3f SpecularBsdf::weight(SurfaceScatterEvent& event) const {
    Vector3f wr = 2 * (event.wi.dot(event.normal)) * event.normal - event.wi;
    if(wr.dot(event.wo) > 0.999f) return Vector3f(1.0f, 1.0f, 1.0f);
    else return Vector3f(0.0f, 0.0f, 0.0f);
  }

  void SpecularBsdf::sample(SurfaceScatterEvent& event) const {
    event.wi = 2 * (event.wo.dot(event.normal)) * event.normal - event.wo; // perfect reflection
    event.pdf = 1.0f;
    event.weight = Vector3f(1.0f, 1.0f, 1.0f);
  }

} 