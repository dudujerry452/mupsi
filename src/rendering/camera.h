#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <Eigen/Core>
#include "geometry/ray.h"

using namespace Eigen;

namespace mupsi
{
  class Camera
  {
  public:
    Camera(const Vector3f &position, const Vector3f &direction, const Vector3f &up, float fov, int w, int h)
        : position_(position), direction_(direction.normalized()), up_(up.normalized()), fov_(fov), aspect_ratio_((float)w/(float)h), width_(w), height_(h)
    {
      right_ = direction_.cross(up_).normalized();
      true_up_ = right_.cross(direction_).normalized();
    }
    virtual ~Camera() = default;

    Ray generateRay(int x, int y) const; // x, y in [0, 1], (0, 0) is left-bottom corner
    int width() const { return width_; }
    int height() const { return height_; }

  private:
    Vector3f position_, direction_, up_; // dir, up is unit vector
    float fov_;
    float aspect_ratio_;
    int width_, height_;

    Vector3f right_, true_up_; // 相机坐标系的右向量和真实的上向量
  };
}

#endif