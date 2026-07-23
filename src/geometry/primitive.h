#ifndef __PRIMITIVE_H__
#define __PRIMITIVE_H__

#include "ray.h"
#include "intersection.h"
#include "bsdf/bsdf.h"

#include <memory>

namespace mupsi {

class Medium; 
class Bsdf; 
class Texture; 

class Primitive {

  std::shared_ptr<Medium> intMedium_; 
  std::shared_ptr<Medium> extMedium_;

protected: 

  static std::shared_ptr<Bsdf> default_bsdf_; 
  static std::shared_ptr<Texture> default_emission_; 

  Matrix4f transform_; 
  Matrix4f invTransform_;

public: 
  Primitive() = default;
  virtual ~Primitive() = default; 


  virtual bool intersect(Ray& ray, IntersectionTemporary& data) const = 0; 
  virtual void intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const = 0;
  virtual bool occluded(const Ray& ray)const = 0; 

  virtual bool sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const = 0; 
  virtual void prepareForRender() = 0;

  virtual int bsdfNum() const = 0; 
  virtual const Bsdf* getBsdf(int index) const = 0;

  friend class Renderer; 
  friend class Scene; 
}; 

}


#endif 