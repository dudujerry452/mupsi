#ifndef _SPHERE_H_
#define _SPHERE_H_

#include "geometry/primitive.h"

namespace mupsi {

struct SphereIntersection {
  bool backSide;
};

class Sphere : public Primitive {

  std::shared_ptr<Bsdf> bsdf_;
  std::shared_ptr<Texture> emission_;

  // Normal transform matrix = (M^{-1})^{T} upper 3x3, for transforming
  // local-space normals to world space.
  Matrix3f normalTransform_;

private:

  // Compute UV on unit sphere at origin in local space.
  Vector2f getUV(const Vector3f& pLocal) const {
    Vector3f n = pLocal.normalized();
    return Vector2f(std::atan2(n.y(), n.x()) / (2*M_PI) + 0.5f,
                    std::acos(std::clamp(n.z(), -1.0f, 1.0f)) / M_PI);
  }

public :

  // Default: unit sphere at origin in local space. Transform = Identity.
  Sphere(): bsdf_(Primitive::default_bsdf_), emission_(nullptr) {
    normalTransform_ = Matrix3f::Identity();
  }

  // Convenience: unit sphere at origin, placed via Translation(center) * Scaling(radius).
  Sphere(const Vector3f& center, float radius, std::shared_ptr<Bsdf> bsdf):
      Sphere() {
    if (bsdf) {
      bsdf_ = bsdf;
    }
    setTransform(Affine3f(Translation3f(center) * Scaling(radius)).matrix());
  }

  virtual ~Sphere() = default;

  bool intersect(Ray& ray, IntersectionTemporary& data) const override;
  void intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const override;
  bool occluded(const Ray& ray) const override;

  bool sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const override;
  void prepareForRender() override;

  int bsdfNum() const override { return 1; }
  const Bsdf* getBsdf(int index) const override;
  const Texture* getEmission() const override { return emission_.get(); }

  void setEmission(std::shared_ptr<Texture> emission) override { emission_ = emission; }

  // Ray-transform: unit sphere at origin in local space.
  // transform_ places it in the world; intersect transforms the ray instead.
  void setTransform(const Matrix4f& transform) override {
    Primitive::setTransform(transform);
    normalTransform_ = invTransform_.topLeftCorner<3,3>().transpose();
  }

};

}
#endif
