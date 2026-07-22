#include "sphere.h"

namespace mupsi {

  void Sphere::intersect(Ray& ray, IntersectionTemporary& data) const {
      Vector3f p = center_ - ray.origin();
      ray.farT = p.dot(ray.direction());
      float dis2 = p.squaredNorm() - ray.farT * ray.farT;
      if (dis2 > radius_ * radius_) {
          return;
      } else {
        
        float offset = sqrt(radius_ * radius_ - dis2);
        ray.farT -= offset;
        if (ray.farT < 0) {
          ray.farT += 2 * offset;
          data.as<SphereIntersection>()->backside = true;
        } else {
          data.as<SphereIntersection>()->backside = false;
        }
        data.primitive = this;

      }
  }
  void Sphere::intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const{

    info.primitive = this;
    info.t = info.t;
    info.p = info.p;
    info.Ng = (info.p - center_).normalized();
    info.bsdf = getBsdf(0);
  }

  const Bsdf* Sphere::getBsdf(int index) const {
    return bsdf_.get();
  }
}