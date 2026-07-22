#include "camera.h"

using namespace mupsi;

Ray Camera::generateRay(int x, int y) const
{
  float cx = (x + 0.5f) / width_;
  float cy = (y + 0.5f) / height_;

  float ndc_x = (2.0f * cx - 1.0f) * aspect_ratio_ * std::tan(fov_ / 2.0f);
  float ndc_y = (2.0f * cy - 1.0f) * std::tan(fov_ / 2.0f);

  Vector3f ray_dir_camera(ndc_x, ndc_y, 1.0f); // suppose looks at -Z
  ray_dir_camera.normalize();

  // transform to world space
  Vector3f ray_dir_world = (right_ * ray_dir_camera.x() + true_up_ * ray_dir_camera.y() + direction_ * ray_dir_camera.z()).normalized();

  return Ray(position_, ray_dir_world.normalized());
}