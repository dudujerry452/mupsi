#ifndef _KERNEL_H_ 
#define _KERNEL_H_ 

#include <Eigen/Core>

using namespace Eigen; 


namespace mupsi {


class SEKernel {

  
  public: 
  SEKernel(float sigma, float kernelRadius, const Vector3f& lengthScale):sigma(sigma), kernelRadius(kernelRadius), lengthScale(lengthScale){}; 

  float h(const Vector3f& s, const Vector3f& p) const {
    return exp(-(p-s).squaredNorm() / (lengthScale.squaredNorm()));  // TODO: 实现马氏距离
  }

  float kappa(const Vector3f& p, const Vector3f& q) const {
    return exp(-(p-q).squaredNorm() / (4 * lengthScale.squaredNorm())); 
  }

  float var(float impulseDensity) const {
    return (impulseDensity / std::pow(kernelRadius, 3)) * std::pow(M_PI, 1.5); 
  }


  float getSigma() const { return sigma; }
  float getKernelRadius() const { return kernelRadius; }
  Vector3f getLengthScale() const { return lengthScale; }  


  private: 

  float sigma; 
  float kernelRadius; 
  Vector3f lengthScale; 
  
}; 

}

#endif 