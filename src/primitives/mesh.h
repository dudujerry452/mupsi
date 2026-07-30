#ifndef _MESH_H_
#define _MESH_H_

#include "geometry/primitive.h"
#include "bvh/bvh.h"
#include <Eigen/Core>
#include <vector>
#include <string>
#include <memory>

using namespace Eigen;

namespace mupsi {

class Mesh : public Primitive {
public:
  Mesh() {normalTransform_ = invTransform_.topLeftCorner<3,3>().transpose(); }
  ~Mesh() = default;

  Mesh(std::shared_ptr<Bsdf> bsdf)
      : Mesh() { bsdf_ = bsdf; }

  // Load OBJ file. Returns true on success.
  bool fetchFrom(const std::string& filename);

  // Set object-to-world transform. Also computes normal transform = (M^{-1})^T.
  void setTransform(const Matrix4f& transform) override {
    Primitive::setTransform(transform);
    normalTransform_ = invTransform_.topLeftCorner<3,3>().transpose();
  }

  // --- Primitive interface ---
  bool intersect(Ray& ray, IntersectionTemporary& data) const override;
  void intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const override;
  bool occluded(const Ray& ray) const override;
  bool sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const override;
  void prepareForRender() override;

  int bsdfNum() const override { return 1; }
  const Bsdf* getBsdf(int index) const override;
  const Texture* getEmission() const override { return emission_.get(); }
  void setEmission(std::shared_ptr<Texture> emission) override { emission_ = emission; }

  // --- Mesh interface ---
  float distToPoint(const Vector3f& p, uint32_t& triIndex, Vector3f& out_p) const;
  bool inside(const Vector3f& p) const; 

  friend class MeshMeanFunction; // allow access to dist2ToPoint and inside

private:
  // --- Geometry data (loaded from OBJ) ---
  std::vector<Vector3f> vertices_;
  std::vector<Vector3f> normals_;       // per-vertex normals (from OBJ or computed)
  std::vector<Vector2f> texcoords_;     // per-vertex texture coordinates
  std::vector<Vector3i> faces_;         // (v0, v1, v2) indices into vertices_
  std::vector<Vector3i> faceTexIndices_; // (t0, t1, t2) indices into texcoords_ (empty if none)
  std::vector<Vector3f> faceNormals_;   // precomputed per-face geometric normals

  // --- Acceleration ---
  BVH bvh_;

  // --- Transform (normal transform = (M^{-1})^T upper 3x3) ---
  Matrix3f normalTransform_;

  // --- Material (single BSDF + emission for entire mesh) ---
  std::shared_ptr<Bsdf> bsdf_;
  std::shared_ptr<Texture> emission_;

  // --- Light sampling data (computed in prepareForRender) ---
  std::vector<float> faceCdf_;  // area-weighted CDF for random face selection
  float totalArea_ = 0.0f;

  // --- Helpers ---
  void computeFaceNormals();
  float computeFaceArea(uint32_t faceIdx) const;
  Vector3f interpolateNormal(uint32_t faceIdx, float u, float v) const;
  Vector2f interpolateTexcoord(uint32_t faceIdx, float u, float v) const;
};

} // namespace mupsi

#endif
