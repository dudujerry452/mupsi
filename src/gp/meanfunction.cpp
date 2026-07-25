#include "meanfunction.h"
#include <Eigen/Core> 

using namespace Eigen; 

namespace mupsi 
{


  float SphereMeanFunction::eval(const Vector3f& p) const
  {
    return (p - center_).norm() - radius_;
  }

  
} // namespace mupsi 
