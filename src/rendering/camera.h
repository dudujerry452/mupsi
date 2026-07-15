#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <eigen3/Eigen/Core>
#include "../geometry/ray.h"

using namespace Eigen;

namespace mupsi
{
  class Camera
  {
  public:
    Camera(const Vector3f &position, const Vector3f &direction, const Vector3f &up, float fov, float aspect_ratio)
        : position(position), direction(direction.normalized()), up(up.normalized()), fov(fov), aspect_ratio(aspect_ratio)
    {
      right = direction.cross(up).normalized();
      true_up = right.cross(direction).normalized();
    }
    virtual ~Camera() = default;

    Ray generateRay(float x, float y) const; // x, y in [0, 1], (0, 0) is left-bottom corner

  private:
    Vector3f position, direction, up; // dir, up is unit vector
    float fov;
    float aspect_ratio;

    Vector3f right, true_up; // 相机坐标系的右向量和真实的上向量
  };
}

#endif