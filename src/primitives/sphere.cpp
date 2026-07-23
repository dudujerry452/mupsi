#include "sphere.h"
#include "math/sample.h"
#include "math/random.h"

namespace mupsi {

  // TODO: 球在后方bug
  void Sphere::intersect(Ray& ray, IntersectionTemporary& data) const { 
      Vector3f p = center_ - ray.origin();
      float hit = p.dot(ray.direction()); 
      hit = p.dot(ray.direction());
      float dis2 = p.squaredNorm() - hit * hit;
      if (dis2 > radius_ * radius_) {
          data.primitive = nullptr;
          return;
      } else {
        
        float offset = sqrt(radius_ * radius_ - dis2);
        hit -= offset;
        if (hit < 0) {
          hit += 2 * offset;
          if (hit < 0) {
            data.primitive = nullptr;
            return;
          }
          data.as<SphereIntersection>()->backside = true;
        } else {
          data.as<SphereIntersection>()->backside = false;
        }
        data.primitive = this;
        ray.farT = hit;
      }
  }
  void Sphere::intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const{

    info.primitive = this;
    info.t = info.t;
    info.p = info.p;
    info.Ng = (info.p - center_).normalized();
    info.bsdf = getBsdf(0);
  }

  bool Sphere::sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const {
    Vector3f dir = center_ - p;
    float dist2 = dir.squaredNorm();
    float d2 = dist2 - radius_ * radius_;
    if (d2 < 0) return false;  // in the sphere
    float d = sqrt(d2);

    float dist = sqrt(dist2);
    dir /= dist; // normalize

    float cosTheta = d / dist; 
    sample.d = SampleWrap::uniformSphericalCap(sampler.next2D(), cosTheta);

    float B = dist * sample.d.z(); 
    float det = std::sqrt(std::max(B * B - d2, 0.0f));
    sample.dist = B - det; 

    sample.d = TangentFrame(dir).toGlobal(sample.d);
    sample.pdf = SampleWrap::uniformSphericalCapPdf(cosTheta);

    return true; 
  }

  const Bsdf* Sphere::getBsdf(int index) const {
    return bsdf_.get();
  }
}