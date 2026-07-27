#ifndef _BVH_H_ 
#define _BVH_H_

#include <vector>
#include <memory>
#include "geometry/intersection.h"
#include "geometry/primitive.h"
#include "geometry/ray.h"
#include "boudingbox.h"

namespace mupsi {

class BVH {


public: 
  BVH() = default; 
  ~BVH() = default; 

  // mainly use for Triangle: Primitive. note: Triangle is for mesh, and mesh is a Primitive.
  void build(const std::vector<std::shared_ptr<Primitive>>& primitives); 
  bool intersect(Ray& ray, IntersectionTemporary& data) const; 
  bool occluded(const Ray& ray) const;

}; 


}


#endif 