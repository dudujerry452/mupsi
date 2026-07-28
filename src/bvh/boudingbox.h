#ifndef _BOUNDING_BOX_H_
#define _BOUNDING_BOX_H_

#include <Eigen/Core>

using namespace Eigen;

namespace mupsi {

class Ray;

class AABB {
public:
  AABB(): minpos(Vector3f::Zero()), maxpos(Vector3f::Zero()) {};
  AABB(const Vector3f& minpos, const Vector3f& maxpos): minpos(minpos), maxpos(maxpos) {};

  static AABB empty() {
    return AABB(
      Vector3f::Constant( INFINITY),
      Vector3f::Constant(-INFINITY)
    );
  }

  void extend(const Vector3f& p) {
    minpos = minpos.cwiseMin(p);
    maxpos = maxpos.cwiseMax(p);
  }

  Vector3f centroid() const { return (minpos + maxpos) * 0.5f; }
  Vector3f diagonal() const { return maxpos - minpos; }

  float surfaceArea() const {
    Vector3f d = diagonal();
    return 2.0f * (d.x()*d.y() + d.y()*d.z() + d.z()*d.x());
  }

  int longestAxis() const {
    Vector3f d = diagonal();
    if (d.x() > d.y() && d.x() > d.z()) return 0;
    if (d.y() > d.z()) return 1;
    return 2;
  }

  const Vector3f& min() const { return minpos; }
  const Vector3f& max() const { return maxpos; }

  bool isValid() const {
    return minpos.x() <= maxpos.x() && minpos.y() <= maxpos.y() && minpos.z() <= maxpos.z();
  }

  // Intersection: max of mins, min of maxes
  AABB operator*(const AABB& other) const {
    return AABB(minpos.cwiseMax(other.minpos), maxpos.cwiseMin(other.maxpos));
  }
  // Union: min of mins, max of maxes
  AABB operator+(const AABB& other) const {
    return AABB(minpos.cwiseMin(other.minpos), maxpos.cwiseMax(other.maxpos));
  }

  bool intersect(const Ray& ray) const;

private:
  Vector3f minpos, maxpos;
};

}

#endif
