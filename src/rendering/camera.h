#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <eigen3/Eigen/Core>

using namespace Eigen;

namespace mupsi
{
  class Camera
  {
  public:
    Camera(const Vector3f &position, const Vector3f &direction, const Vector3f &up, float fov, float aspect_ratio, float znear, float zfar)
        : position(position), direction(direction.normalized()), up(up.normalized()), fov(fov), aspect_ratio(aspect_ratio), znear(znear), zfar(zfar) {}
    virtual ~Camera() = default;

    private:
    Vector3f position, direction, up; // dir, up is unit vector
    float fov;
    float aspect_ratio;
    float znear, zfar;
  };
}

#endif