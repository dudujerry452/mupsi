#ifndef _SPHERE_H_
#define _SPHERE_H_

#include "geometry/primitive.h"

namespace mupsi {

struct SphereIntersection {
  bool backside; 
};

class Sphere : public Primitive {

  Vector3f center_;
  float radius_;

  std::shared_ptr<Bsdf> bsdf_;

public : 

  Sphere():center_(Vector3f(0, 0, 0)), radius_(1.0f), bsdf_(Primitive::default_bsdf_){}
  Sphere(const Vector3f& center, float radius, std::shared_ptr<Bsdf> bsdf): center_(center), radius_(radius){
    if(bsdf) {
      bsdf_ = bsdf; 
    } else {
      bsdf_ = Primitive::default_bsdf_; 
    }
  }; 
  virtual ~Sphere() = default; 

  void intersect(Ray& ray, IntersectionTemporary& data) const override; 
  void intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const override;

  bool sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const; 

  int bsdfNum() const override { return 1; } 
  const Bsdf* getBsdf(int index) const override;


}; 
}



#endif 