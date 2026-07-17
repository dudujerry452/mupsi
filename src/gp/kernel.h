#ifndef _KERNEL_H_ 
#define _KERNEL_H_ 

#include <Eigen/Core>

using namespace Eigen; 


namespace mupsi {


class SEKernel {

  float length_scale; 
  public: 
  SEKernel(float length_scale):length_scale(length_scale) {}; 

  float h(const Vector3f& s, const Vector3f& p) const {
    return exp(-(p-s).squaredNorm() / (length_scale * length_scale)); 
  }

  float kappa(const Vector3f& p, const Vector3f& q) const {
    return exp(-(p-q).squaredNorm() / (4 * length_scale * length_scale)); 
  }
}; 

}

#endif 