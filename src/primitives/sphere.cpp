#include "sphere.h"
#include "math/sample.h"
#include "math/random.h"

namespace mupsi {

bool Sphere::intersect(Ray& ray, IntersectionTemporary& data) const { 
    Vector3f p = ray.origin() - center_;
    float B = p.dot(ray.direction());
    float C = p.squaredNorm() - radius_*radius_;
    float detSq = B*B - C;
    if (detSq >= 0.0f) {
        float det = std::sqrt(detSq);
        float t = -B - det;
        if (t < ray.farT() && t > ray.nearT()) {
            ray.setFarT(t);
            data.primitive = this;
            data.as<SphereIntersection>()->backSide = false;
            return true;
        }
        t = -B + det;
        if (t < ray.farT() && t > ray.nearT()) {
            ray.setFarT(t);
            data.primitive = this;
            data.as<SphereIntersection>()->backSide = true;
            return true;
        }
    }
    return false;
  }

  void Sphere::intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const{

    info.primitive = this;
    info.t = info.t;
    info.p = info.p;
    info.Ng = (info.p - center_).normalized();

    Vector3f &localN = info.Ng;  // TODO: add transform matrix 
    info.uv = Vector2f(std::atan2(localN.y(), localN.x()) / (2*M_PI) + 0.5f, std::acos(std::clamp(localN.z(), -1.0f, 1.0f)) / M_PI);
    if (std::isnan(info.uv.x()))
      info.uv.x() = 0.0f;
    info.bsdf = getBsdf(0);
  }

  bool Sphere::occluded(const Ray& ray) const {
    Vector3f p = ray.origin() - center_;
    float B = p.dot(ray.direction());
    float C = p.squaredNorm() - radius_*radius_;
    float detSq = B*B - C;
    if (detSq >= 0.0f) {
        float det = std::sqrt(detSq);
        float t = -B - det;
        if (t < ray.farT() && t > ray.nearT()) {
            return true;
        }
        t = -B + det;
        if (t < ray.farT() && t > ray.nearT()) {
            return true;
        }
    }
    return false;
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

  void Sphere::prepareForRender() {
    
  }

  const Bsdf* Sphere::getBsdf(int index) const {
    return bsdf_.get();
  }
}