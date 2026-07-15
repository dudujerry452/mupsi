#include "camera.h"

using namespace mupsi;

Ray Camera::generateRay(float x, float y) const
{
  float ndc_x = (2.0f * x - 1.0f) * aspect_ratio * std::tan(fov / 2.0f);
  float ndc_y = (2.0f * y - 1.0) * std::tan(fov / 2.0f);

  Vector3f ray_dir_camera(ndc_x, ndc_y, -1.0f); // suppose looks at -Z
  ray_dir_camera.normalize();

  // transform to world space
  Vector3f ray_dir_world = (right * ray_dir_camera.x() + true_up * ray_dir_camera.y() + direction * ray_dir_camera.z()).normalized();

  return Ray(position, ray_dir_world);
}