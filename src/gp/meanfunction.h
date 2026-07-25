#ifndef _MEANFUNCTION_H_
#define _MEANFUNCTION_H_

#include <Eigen/Core>
using namespace Eigen;

namespace mupsi {
  class MeanFunction {

    public: 

    virtual ~MeanFunction() = default;
    virtual float eval(const Vector3f& p) const = 0; 
    virtual Vector3f gradient(const Vector3f& p) const = 0;
  }; 

  class SphereMeanFunction : public MeanFunction {
    Vector3f center_;
    float radius_;

    public: 

    SphereMeanFunction(const Vector3f& center, float radius) : center_(center), radius_(radius) {}
    float eval(const Vector3f& p) const override; 
    Vector3f gradient(const Vector3f& p) const override;
  };



}

#endif 