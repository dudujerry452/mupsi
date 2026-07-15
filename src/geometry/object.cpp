#include "object.hpp"

using namespace mupsi;

SDFSphere::SDFSphere(Vector3f center, float radius) : center(center), radius(radius)
{
}

float SDFSphere::eval(const Vector3f &point) const
{
  return (point - center).norm() - radius;
}