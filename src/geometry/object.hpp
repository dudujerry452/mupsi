#ifndef _SDF_HPP_
#define _SDF_HPP_

#include <eigen3/Eigen/Eigen>
using namespace Eigen;

namespace mupsi
{
  class Object
  {
  public:
    Object() = default;
    virtual ~Object() = default;

  private:
  };

  class SDFObject : public Object
  {
  public:
    SDFObject() = default;
    virtual ~SDFObject() = default;

    virtual float eval(const Vector3f &point) const = 0;
  };

  class SDFSphere : public SDFObject
  {
  public:
    SDFSphere(const Vector3f &center, float radius);
    virtual ~SDFSphere() = default;

    float eval(const Vector3f &point) const override;

  private:
    Vector3f center;
    float radius;
  };
}

#endif