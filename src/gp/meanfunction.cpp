#include "meanfunction.h"
#include <Eigen/Core> 

using namespace Eigen; 

namespace mupsi 
{


  float SphereMeanFunction::eval(const Vector3f& p) const
  {
    return (p - center_).norm() - radius_;
  }
  Vector3f SphereMeanFunction::gradient(const Vector3f& p) const {
    Vector3f diff = p - center_;
    return diff.normalized();
  }

  
} // namespace mupsi 
