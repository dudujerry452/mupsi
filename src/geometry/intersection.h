#ifndef _INTERSECTION_H_
#define _INTERSECTION_H_

#include <Eigen/Core>

using namespace Eigen;

namespace mupsi {

class Primitive; 

struct IntersectionTemporary {

  const Primitive* primitive; 
  uint8_t data[64];  // 64bytes 

  IntersectionTemporary() = default; 

  template<typename T> 
  T* as() { 
    static_assert(sizeof(T) <= sizeof(data), "IntersectionTemporary::data is too small for type T");
    return reinterpret_cast<T*>(data);
  }
  template<typename T> 
  const T* as() const { 
    static_assert(sizeof(T) <= sizeof(data), "IntersectionTemporary::data is too small for type T");
    return reinterpret_cast<T*>(data);
  }
}; 

class Bsdf;

struct IntersectionInfo {

  Vector3f Ng; 
  Vector3f p; 
  float t;

  const Primitive* primitive;
  const Bsdf* bsdf; 

  // medium
  uint32_t sceneSeed; 
  float marchingT;
};
}

#endif 