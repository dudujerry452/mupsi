#include "object.h"

using namespace mupsi;

SDFSphere::SDFSphere(const Vector3f &center, float radius, std::shared_ptr<Material> material) : SDFObject(material), center(center), radius(radius)
{
  bounding = Bounding(center - Vector3f(radius, radius, radius), center + Vector3f(radius, radius, radius));
}

float SDFSphere::eval(const Vector3f &point) const
{
  return (point - center).norm() - radius;
}

SDFCube::SDFCube(const Vector3f &center, const Vector3f &size, std::shared_ptr<Material> material)
    : SDFObject(material), center(center), halfSize(size * 0.5f)
{
  bounding = Bounding(center - halfSize, center + halfSize);
}

float SDFCube::eval(const Vector3f &point) const
{
  Vector3f q = (point - center).cwiseAbs() - halfSize;
  return std::max(q.x(), std::max(q.y(), q.z()));
}
