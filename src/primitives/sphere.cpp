#include "sphere.h"
#include "math/sample.h"
#include "math/sampler.h"
#include "texture/texture.h"
#include <Eigen/Geometry>

namespace mupsi {

// Ray-sphere intersection with full quadratic (non-unit direction).
// Returns t (in units of the input direction D), or -1 on miss.
static float intersectUnitSphere(float nearT, float farT,
                                 const Vector3f& O, const Vector3f& D) {
  float a = D.squaredNorm();
  float b = O.dot(D);
  float c = O.squaredNorm() - 1.0f; // unit sphere at origin

  float disc = b*b - a*c;
  if (disc < 0.0f) return -1.0f;

  float sqrtDisc = std::sqrt(disc);

  // t1 = (-b - sqrtDisc) / a  (nearer intersection)
  float t = (-b - sqrtDisc) / a;
  if (t > nearT && t < farT) return t;

  t = (-b + sqrtDisc) / a;
  if (t > nearT && t < farT) return t;

  return -1.0f;
}

bool Sphere::intersect(Ray& ray, IntersectionTemporary& data) const {
  Vector3f O = ray.origin();
  Vector3f D = ray.direction();

  if (!invTransform_.isIdentity()) {
    Vector4f o4 = invTransform_ * Vector4f(O.x(), O.y(), O.z(), 1.0f);
    O = o4.head<3>();
    D = invTransform_.topLeftCorner<3,3>() * D; // unnormalized — preserves scale in t
  }

  float t = intersectUnitSphere(ray.nearT(), ray.farT(), O, D);
  if (t < 0.0f) return false;

  // Determine backSide: check if the local normal faces toward +D or -D.
  Vector3f pLocal = O + t * D; // local hit point
  Vector3f localNg = pLocal.normalized();
  bool backSide = localNg.dot(D) > 0.0f; // entering sphere → normal points opposite to D

  ray.setFarT(t);
  data.primitive = this;
  data.as<SphereIntersection>()->backSide = backSide;
  return true;
}

void Sphere::intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const {
  info.primitive = this;
  info.t = info.t;
  info.p = info.p; // already world-space from Scene

  // info.p is world-space. Get local-space point then compute normal there.
  Vector3f pLocal = invTransform_.isIdentity()
                    ? info.p
                    : (invTransform_ * Vector4f(info.p.x(), info.p.y(), info.p.z(), 1.0f)).head<3>();
  Vector3f localNg = pLocal.normalized();

  if (!invTransform_.isIdentity()) {
    info.Ng = normalTransform_ * localNg;
    info.Ng.normalize();
  } else {
    info.Ng = localNg;
  }

  info.uv = getUV(pLocal);
  info.bsdf = getBsdf(0);
}

bool Sphere::occluded(const Ray& ray) const {
  Vector3f O = ray.origin();
  Vector3f D = ray.direction();

  if (!invTransform_.isIdentity()) {
    Vector4f o4 = invTransform_ * Vector4f(O.x(), O.y(), O.z(), 1.0f);
    O = o4.head<3>();
    D = invTransform_.topLeftCorner<3,3>() * D;
  }

  return intersectUnitSphere(ray.nearT(), ray.farT(), O, D) >= 0.0f;
}

bool Sphere::sampleDirect(const Vector3f& pWorld, Sampler& sampler, LightSample& sample) const {
  // Transform query point to local space
  Vector3f pLocal = pWorld;
  if (!invTransform_.isIdentity()) {
    Vector4f tmp = invTransform_ * Vector4f(pWorld.x(), pWorld.y(), pWorld.z(), 1.0f);
    pLocal = tmp.head<3>();
  }

  // Unit sphere at origin, radius 1
  Vector3f dir = -pLocal;
  float dist2 = dir.squaredNorm();
  float d2 = dist2 - 1.0f;
  if (d2 < 0) return false;

  float d = std::sqrt(d2);
  float dist = std::sqrt(dist2);
  dir /= dist;

  float cosTheta = d / dist;
  sample.d = SampleWrap::uniformSphericalCap(sampler.next2D(), cosTheta);

  float B = dist * sample.d.z();
  float det = std::sqrt(std::max(B * B - d2, 0.0f));
  sample.dist = B - det;

  sample.d = TangentFrame(dir).toGlobal(sample.d);

  // If there's a transform, convert from local to world
  if (!invTransform_.isIdentity()) {
    Vector3f sampledPtLocal = pLocal + sample.d * sample.dist;
    Vector4f sampledPtWorld4 = transform_ * Vector4f(sampledPtLocal.x(), sampledPtLocal.y(), sampledPtLocal.z(), 1.0f);
    Vector3f sampledPtWorld = sampledPtWorld4.head<3>();
    Vector3f worldDir = sampledPtWorld - pWorld;
    sample.dist = worldDir.norm();
    sample.d = worldDir / sample.dist;
  }

  sample.pdf = SampleWrap::uniformSphericalCapPdf(cosTheta);
  Vector2f uv = getUV(pLocal);
  sample.weight = (emission_ ? (*emission_)[uv] : Vector3f::Zero())
                / std::max(sample.pdf, 1e-6f);
  return true;
}

void Sphere::prepareForRender() {
}

const Bsdf* Sphere::getBsdf(int /*index*/) const {
  return bsdf_.get();
}

}
