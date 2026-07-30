#ifndef _BVH_H_
#define _BVH_H_

#include <cstdint>
#include <vector>
#include <Eigen/Core>
#include "boudingbox.h"

using namespace Eigen;

namespace mupsi {

class Ray;
class IntersectionTemporary;

// Per-triangle hit data stored in IntersectionTemporary::data[64]
struct MeshIntersection {
  uint32_t triIndex;
  float u, v; // barycentric coordinates: w = 1-u-v
};

class BVH {
public:
  static constexpr uint32_t LEAF_FLAG = 0x80000000u;

  struct Node {
    AABB bounds;
    // Leaf: left = startIdx, right = triCount | LEAF_FLAG
    // Internal: left = leftChildIdx, right = rightChildIdx (no flag)
    uint32_t left  = 0;
    uint32_t right = 0;

    bool isLeaf() const { return (right & LEAF_FLAG) != 0; }
    uint32_t leafStart() const { return left; }
    uint32_t leafCount() const { return right & ~LEAF_FLAG; }
  };

  BVH() = default;
  ~BVH() = default;

  // Build from mesh data. vertices and faces must remain valid for the lifetime of the BVH.
  // faces[i] = Vector3i(v0, v1, v2) where v0,v1,v2 index into vertices.
  void build(const Vector3f* vertices, const Vector3i* faces, uint32_t triCount);

  // Traverse BVH. On triangle hit, narrows ray.farT() and fills
  // data.as<MeshIntersection>() with triangle index + barycentrics.
  // Does NOT set data.primitive.
  bool intersect(Ray& ray, IntersectionTemporary& data) const;

  // Early-exit shadow ray test. Returns true if any triangle occludes.
  bool occluded(const Ray& ray) const;

  // Find closest point on mesh to p. If not found, return inf
  float closest(const Vector3f& p, uint32_t& triIndex, Vector3f& out_p) const; 

  // Debug: brute-force intersect — loop every triangle, no BVH.
  bool intersectBruteForce(Ray& ray, IntersectionTemporary& data) const;

  uint32_t rootIndex_ = 0;

private:
  std::vector<Node> nodes_;
  std::vector<uint32_t> triIndices_; // reordered for spatial locality

  // Non-owning pointers to mesh data
  const Vector3f* vertices_ = nullptr;
  const Vector3i* faces_    = nullptr;
  uint32_t triCount_ = 0;

  // Recursive build helper
  uint32_t buildRecursive(uint32_t begin, uint32_t end);

  // Compute AABB for a single triangle
  static AABB triBounds(const Vector3f* v, const Vector3i& f);

  // Möller-Trumbore ray-triangle intersection
  static bool rayTriIntersect(const Ray& ray,
                              const Vector3f& v0, const Vector3f& v1, const Vector3f& v2,
                              float& t, float& u, float& v);

};

} // namespace mupsi

#endif
