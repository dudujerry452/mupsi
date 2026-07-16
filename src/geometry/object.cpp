#include "object.h"

using namespace mupsi;

SDFSphere::SDFSphere(const Vector3f &center, float radius) : center(center), radius(radius)
{
  bounding = Bounding(center - Vector3f(radius, radius, radius), center + Vector3f(radius, radius, radius));
}

float SDFSphere::eval(const Vector3f &point) const
{
  return (point - center).norm() - radius;
}