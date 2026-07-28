#ifndef _SKYDROME_H_
#define _SKYDROME_H_

#include "geometry/primitive.h"
#include "texture/texture.h"

namespace mupsi { 

struct SkydromeIntersection {
  float u, v; 
};

class Skydrome : public Primitive {

  static std::shared_ptr<Texture> default_emission_;
  std::shared_ptr<Texture> emission_;

  public: 
  Skydrome(): emission_(default_emission_) {};
  Skydrome(std::shared_ptr<Texture> emission): emission_(emission) {};

  bool intersect(Ray& ray, IntersectionTemporary& data) const; 
  void intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const;
  bool occluded(const Ray& ray)const {return false; }

  bool sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const { return false; }
  void prepareForRender() {}

  int bsdfNum() const { return 0; }
  const Bsdf* getBsdf(int index) const {return nullptr;}  
  const Texture* getEmission() const {return emission_.get(); } 

  void setEmission(std::shared_ptr<Texture> emission) { emission_ = emission; }

}; 

}

#endif 