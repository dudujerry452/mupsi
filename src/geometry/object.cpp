#include "object.h"

using namespace mupsi;

SDFSphere::SDFSphere(const Vector3f &center, float radius) : center(center), radius(radius)
{
}

float SDFSphere::eval(const Vector3f &point) const
{
  return (point - center).norm() - radius;
}