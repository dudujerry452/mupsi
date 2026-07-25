#ifndef _RAY_H_
#define _RAY_H_

#include <Eigen/Core>
using namespace Eigen;

namespace mupsi
{
  class Ray
  {
  public:
    Ray(const Vector3f &origin, const Vector3f &direction) : origin_(origin), direction_(direction.normalized()), nearT_(0.0f), farT_(std::numeric_limits<float>::max()) {}
    virtual ~Ray() = default;

    const Vector3f origin() const { return origin_; }
    const Vector3f direction() const { return direction_; }

    void setNearT(float t) { nearT_ = t; }
    void setFarT(float t) { farT_ = t; }

    float nearT() const { return nearT_; }
    float farT() const { return farT_; }

  private:
  
    Vector3f origin_, direction_; // dir is unit vector
    float nearT_; // editable
    float farT_; // editable
  };
}

#endif