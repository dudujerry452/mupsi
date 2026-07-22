#ifndef _BOUNDING_BOX_H_ 
#define _BOUNDING_BOX_H_

#include <Eigen/Core>

using namespace Eigen;

namespace mupsi {

class Ray; 

class AABB {
  Vector3f minpos, maxpos; 
  AABB(): minpos(Vector3f::Zero()), maxpos(Vector3f::Zero()) {};
  AABB(const Vector3f& minpos, const Vector3f& maxpos): minpos(minpos), maxpos(maxpos) {};

  Vector3f getInterPos(const Vector3f& pos) const{
    return (pos - minpos); 
  };
  Vector3f getOuterPos(const Vector3f& pos) const{
    return (pos + minpos); 
  };

  AABB operator*(const AABB& other) const{
    return AABB(minpos.cwiseMin(other.minpos), maxpos.cwiseMax(other.maxpos)); 
  }; // 求交
  AABB operator+(const AABB& other) const{
    return AABB(minpos.cwiseMax(other.minpos), maxpos.cwiseMin(other.maxpos)); 
  }; // 求并

  bool intersect(Ray& ray) const; 

}; 
  
}
#endif 