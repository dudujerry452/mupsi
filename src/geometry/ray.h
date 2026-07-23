#ifndef _RAY_H_
#define _RAY_H_

#include <Eigen/Core>
using namespace Eigen;

namespace mupsi
{
  class Ray
  {
  public:
    Ray(const Vector3f &origin, const Vector3f &direction) : farT(std::numeric_limits<float>::max()), origin_(origin), direction_(direction.normalized()) {}
    virtual ~Ray() = default;

    const Vector3f origin() const { return origin_; }
    const Vector3f direction() const { return direction_; }

    float farT; // editable

  private:
    Vector3f origin_, direction_; // dir is unit vector
  };
}

#endif