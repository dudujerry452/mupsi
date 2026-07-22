#ifndef __PRIMITIVE_H__
#define __PRIMITIVE_H__

#include "ray.h"
#include "intersection.h"
#include "bsdf/bsdf.h"

#include <memory>

namespace mupsi {

class Medium; 
class Bsdf; 

class Primitive {

  std::shared_ptr<Medium> intMedium_; 
  std::shared_ptr<Medium> extMedium_;

protected: 

  static std::shared_ptr<Bsdf> default_bsdf_; 

public: 

  virtual ~Primitive() = default; 
  Primitive() = default;

  virtual void intersect(Ray& ray, IntersectionTemporary& data) const = 0; 
  virtual void intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const = 0;

  virtual int bsdfNum() const = 0; 
  virtual const Bsdf* getBsdf(int index) const = 0;
}; 

}


#endif 