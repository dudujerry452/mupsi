#ifndef _SPHERE_H_
#define _SPHERE_H_

#include "geometry/primitive.h"

namespace mupsi {

struct SphereIntersection {
  bool backSide; 
};

class Sphere : public Primitive {

  Vector3f center_;
  float radius_;

  std::shared_ptr<Bsdf> bsdf_;
  std::shared_ptr<Texture> emission_;

public : 

  Sphere():center_(Vector3f(0, 0, 0)), radius_(1.0f), bsdf_(Primitive::default_bsdf_), emission_(nullptr) {
    transform_ = Matrix4f::Identity();
    invTransform_ = Matrix4f::Identity();
  }
  Sphere(const Vector3f& center, float radius, std::shared_ptr<Bsdf> bsdf): 
  center_(center), radius_(radius), emission_(nullptr) {
    if(bsdf) {
      bsdf_ = bsdf; 
    } else {
      bsdf_ = Primitive::default_bsdf_; 
    }
  }

  virtual ~Sphere() = default;

  bool intersect(Ray& ray, IntersectionTemporary& data) const override; 
  void intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const override;
  bool occluded(const Ray& ray) const override;

  bool sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const override; 
  void prepareForRender() override; 

  int bsdfNum() const override { return 1; } 
  const Bsdf* getBsdf(int index) const override;
  const Texture* getEmission() const { return emission_.get(); }

  // set 
  void setEmission(std::shared_ptr<Texture> emission) { emission_ = emission; }

}; 
}



#endif 