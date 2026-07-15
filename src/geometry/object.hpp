#ifndef _SDF_HPP_
#define _SDF_HPP_

#include <eigen3/Eigen/Eigen>
using namespace Eigen;

namespace mupsi
{
  class Object
  {
  public:
    Object();
    virtual ~Object();

  private:
  };

  class SDFObject : public Object
  {
  public:
    SDFObject();
    virtual ~SDFObject();

    virtual float eval(const Vector3f &point) const = 0;
  };

  class SDFSphere : public SDFObject
  {
  public:
    SDFSphere(float radius);
    virtual ~SDFSphere();

    float eval(const Vector3f &point) const override;
  };
}

#endif