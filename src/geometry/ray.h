#ifndef _RAY_H_
#define _RAY_H_

#include <eigen3/Eigen/Core>
using namespace Eigen;

namespace mupsi
{
  class Ray
  {
  public:
    Ray(const Vector3f &origin, const Vector3f &direction) : origin(origin), direction(direction.normalized()) {}
    virtual ~Ray() = default;

  private:
    Vector3f origin, direction; // dir is unit vector
  };
}

#endif