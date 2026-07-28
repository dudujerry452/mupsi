#ifndef __PRIMITIVE_H__
#define __PRIMITIVE_H__

#include "ray.h"
#include "intersection.h"
#include "bsdf/bsdf.h"

#include <Eigen/Dense>
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
  Primitive() {
    transform_    = Matrix4f::Identity();
    invTransform_ = Matrix4f::Identity();
  }
  virtual ~Primitive() = default;

  // Set object-to-world transform. Subclasses may override to bake or use for ray-transform.
  virtual void setTransform(const Matrix4f& transform) {
    transform_    = transform;
    invTransform_ = transform.inverse();
  }
  const Matrix4f& getTransform()    const { return transform_; }
  const Matrix4f& getInvTransform() const { return invTransform_; }

  virtual bool intersect(Ray& ray, IntersectionTemporary& data) const = 0; 
  virtual void intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const = 0;
  virtual bool occluded(const Ray& ray)const = 0; 

  virtual bool sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const = 0; 
  virtual void prepareForRender() = 0;

  virtual int bsdfNum() const = 0; 
  virtual const Bsdf* getBsdf(int index) const = 0;  
  virtual const Texture* getEmission() const = 0; 
  
  virtual void setEmission(std::shared_ptr<Texture> emission) = 0;

  void setMedium(std::shared_ptr<Medium> intMedium, std::shared_ptr<Medium> extMedium) {
    intMedium_ = intMedium; 
    extMedium_ = extMedium; 
  }
  const std::shared_ptr<Medium>& getIntMedium() const { return intMedium_; }
  const std::shared_ptr<Medium>& getExtMedium() const { return extMedium_; }

  friend class Renderer; 
  friend class Scene; 
}; 

}


#endif 