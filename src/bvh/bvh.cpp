#include "bvh.h"
#include "geometry/ray.h"
#include "geometry/intersection.h"
#include <algorithm>
#include <cmath>
#include <Eigen/Geometry>

namespace mupsi {

// --- helpers ---

AABB BVH::triBounds(const Vector3f* v, const Vector3i& f) {
  AABB box = AABB::empty();
  box.extend(v[f.x()]);
  box.extend(v[f.y()]);
  box.extend(v[f.z()]);
  return box;
}

float dist2ToAABB(const Vector3f& p, const AABB& box) {
  float dx = std::max({box.min().x() - p.x(), 0.0f, p.x() - box.max().x()});
  float dy = std::max({box.min().y() - p.y(), 0.0f, p.y() - box.max().y()});
  float dz = std::max({box.min().z() - p.z(), 0.0f, p.z() - box.max().z()});
  return dx*dx + dy*dy + dz*dz;
}

float dist2ToLine(const Vector3f& p, const Vector3f& a, const Vector3f& b, Vector3f& out_p) {
  Vector3f ab = b - a;
  Vector3f ap = p - a; 
  if(ab.squaredNorm() < 1e-6f) {
    out_p = a; 
    return ap.squaredNorm();
  }
  float d = ap.dot(ab); 
  float t = d / ab.squaredNorm();
  t = std::clamp(t, 0.0f, 1.0f);
  Vector3f proj = a + t * ab;
  out_p = proj;
  return (p - proj).squaredNorm(); 
}

float dist2ToTriangle(const Vector3f&p, const Vector3f* v, const Vector3i& f, Vector3f& out_p) {
  const Vector3f& a = v[f.x()];
  const Vector3f& b = v[f.y()];
  const Vector3f& c = v[f.z()];

  const Vector3f edge1 = a - b; 
  const Vector3f edge2 = a - c;
  Vector3f normal = edge1.cross(edge2);
  
  if(normal.squaredNorm() < 1e-6f) {
    // degenerate triangle, return distance to vertex a
    out_p = a; 
    return (p - a).squaredNorm();
  }
  normal.normalize(); 

  float distToPlane = (p - a).dot(normal);
  Vector3f pjd = p - distToPlane * normal;
  
  Vector3f ab = -edge1; 
  Vector3f bc = c - b;
  Vector3f ca = edge2; 
  Vector3f c1 = ab.cross(pjd - a); 
  Vector3f c2 = bc.cross(pjd - b);
  Vector3f c3 = ca.cross(pjd - c);
  if(c1.dot(normal) >= 0 && c2.dot(normal) >= 0 && c3.dot(normal) >= 0) {
    // pjd is inside the triangle
    out_p = pjd;
    return distToPlane * distToPlane;
  } else {
    // pjd is outside the triangle, return distance to closest edge or vertex
    float minn = std::numeric_limits<float>::infinity(), d;
    Vector3f tmp;
    if((d = dist2ToLine(p, a, b, tmp)) < minn) { minn = d; out_p = tmp; }
    if((d = dist2ToLine(p, b, c, tmp)) < minn) { minn = d; out_p = tmp; }
    if((d = dist2ToLine(p, c, a, tmp)) < minn) { minn = d; out_p = tmp; }
    return minn;
  }
}

bool BVH::rayTriIntersect(const Ray& ray,
                          const Vector3f& v0, const Vector3f& v1, const Vector3f& v2,
                          float& t, float& u, float& v) {
  constexpr float eps = 1e-8f;

  Vector3f edge1 = v1 - v0;
  Vector3f edge2 = v2 - v0;
  Vector3f h = ray.direction().cross(edge2);
  float a = edge1.dot(h);

  if (std::abs(a) < eps)
    return false; // ray parallel to triangle

  float f = 1.0f / a;
  Vector3f s = ray.origin() - v0;
  u = f * s.dot(h);

  if (u < 0.0f || u > 1.0f)
    return false;

  Vector3f q = s.cross(edge1);
  v = f * ray.direction().dot(q);

  if (v < 0.0f || u + v > 1.0f)
    return false;

  t = f * edge2.dot(q);

  return t > ray.nearT() && t < ray.farT();
}

// --- build ---

void BVH::build(const Vector3f* vertices, const Vector3i* faces, uint32_t triCount) {
  vertices_ = vertices;
  faces_    = faces;
  triCount_ = triCount;

  if (triCount == 0) return;

  triIndices_.resize(triCount);
  for (uint32_t i = 0; i < triCount; ++i)
    triIndices_[i] = i;

  nodes_.clear();
  nodes_.reserve(2 * triCount);

  rootIndex_ = buildRecursive(0, triCount);
}

uint32_t BVH::buildRecursive(uint32_t begin, uint32_t end) {
  uint32_t count = end - begin;

  // Compute bounding box of all triangles in [begin, end)
  AABB box = AABB::empty();
  for (uint32_t i = begin; i < end; ++i) {
    uint32_t ti = triIndices_[i];
    box = box + triBounds(vertices_, faces_[ti]);
  }

  // Leaf node
  if (count <= 4) {
    uint32_t idx = nodes_.size();
    Node leaf;
    leaf.bounds = box;
    leaf.left   = begin;
    leaf.right  = count | LEAF_FLAG;
    nodes_.push_back(leaf);
    return idx;
  }

  // Internal node: split on longest axis at centroid midpoint
  int axis = box.longestAxis();

  // Compute the midpoint of triangle centroids along the split axis
  float splitMin =  INFINITY;
  float splitMax = -INFINITY;
  for (uint32_t i = begin; i < end; ++i) {
    uint32_t ti = triIndices_[i];
    const Vector3i& f = faces_[ti];
    float c = (vertices_[f.x()][axis] + vertices_[f.y()][axis] + vertices_[f.z()][axis]) / 3.0f;
    splitMin = std::min(splitMin, c);
    splitMax = std::max(splitMax, c);
  }
  float splitPos = (splitMin + splitMax) * 0.5f;

  // Partition: triangles with centroid < splitPos go left
  auto mid = std::partition(triIndices_.begin() + begin, triIndices_.begin() + end,
    [&](uint32_t ti) {
      const Vector3i& f = faces_[ti];
      float c = (vertices_[f.x()][axis] + vertices_[f.y()][axis] + vertices_[f.z()][axis]) / 3.0f;
      return c < splitPos;
    });

  uint32_t midIdx = mid - triIndices_.begin();

  // Degenerate: all triangles fell to one side → split range in half
  if (midIdx == begin || midIdx == end) {
    midIdx = begin + count / 2;
  }

  uint32_t leftChild  = buildRecursive(begin, midIdx);
  uint32_t rightChild = buildRecursive(midIdx, end);

  uint32_t idx = nodes_.size();
  Node internal;
  internal.bounds = box;
  internal.left   = leftChild;
  internal.right  = rightChild;
  nodes_.push_back(internal);
  return idx;
}

// --- intersect ---

bool BVH::intersect(Ray& ray, IntersectionTemporary& data) const {
  if (nodes_.empty()) return false;

  bool hit = false;
  uint32_t stack[64];
  int32_t sp = 0;
  stack[sp++] = rootIndex_;

  while (sp > 0) {
    uint32_t nodeIdx = stack[--sp];
    const Node& node = nodes_[nodeIdx];

    if (!node.bounds.intersect(ray))
      continue;

    if (node.isLeaf()) {
      uint32_t start = node.leafStart();
      uint32_t cnt   = node.leafCount();
      for (uint32_t i = start; i < start + cnt; ++i) {
        uint32_t ti = triIndices_[i];
        const Vector3i& f = faces_[ti];
        float t, u, v;
        if (rayTriIntersect(ray, vertices_[f.x()], vertices_[f.y()], vertices_[f.z()], t, u, v)) {
          ray.setFarT(t);
          auto* mi = data.as<MeshIntersection>();
          mi->triIndex = ti;
          mi->u = u;
          mi->v = v;
          hit = true;
        }
      }
    } else {
      // Internal node: front-to-back traversal
      uint32_t left  = node.left;
      uint32_t right = node.right;

      // Determine near/far child based on ray direction along longest axis
      const Node& ln = nodes_[left];
      const Node& rn = nodes_[right];
      int axis = node.bounds.longestAxis();
      float lc = ln.bounds.centroid()[axis];
      float rc = rn.bounds.centroid()[axis];

      bool nearFirst = (ray.direction()[axis] > 0) ? (lc < rc) : (lc > rc);

      uint32_t first  = nearFirst ? left  : right;
      uint32_t second = nearFirst ? right : left;
      stack[sp++] = second;
      stack[sp++] = first;
    }
  }

  return hit;
}

// --- occluded ---

bool BVH::occluded(const Ray& ray) const {
  if (nodes_.empty()) return false;

  uint32_t stack[64];
  int32_t sp = 0;
  stack[sp++] = 0;

  while (sp > 0) {
    uint32_t nodeIdx = stack[--sp];
    const Node& node = nodes_[nodeIdx];

    if (!node.bounds.intersect(ray))
      continue;

    if (node.isLeaf()) {
      uint32_t start = node.leafStart();
      uint32_t cnt   = node.leafCount();
      for (uint32_t i = start; i < start + cnt; ++i) {
        uint32_t ti = triIndices_[i];
        const Vector3i& f = faces_[ti];
        float t, u, v;
        if (rayTriIntersect(ray, vertices_[f.x()], vertices_[f.y()], vertices_[f.z()], t, u, v)) {
          return true;
        }
      }
    } else {
      uint32_t left  = node.left;
      uint32_t right = node.right;

      // Front-to-back traversal for occluded too (hit sooner)
      const Node& ln = nodes_[left];
      const Node& rn = nodes_[right];
      int axis = node.bounds.longestAxis();
      float lc = ln.bounds.centroid()[axis];
      float rc = rn.bounds.centroid()[axis];

      bool nearFirst = (ray.direction()[axis] > 0) ? (lc < rc) : (lc > rc);

      uint32_t first  = nearFirst ? left  : right;
      uint32_t second = nearFirst ? right : left;
      stack[sp++] = second;
      stack[sp++] = first;
    }
  }

  return false;
}

float BVH::closest(const Vector3f& p, uint32_t& triIndex, Vector3f& out_p) const {

  if (nodes_.empty()) return false;

  uint32_t stack[64];
  int32_t sp = 0;
  stack[sp++] = rootIndex_;
  
  float min_dist = std::numeric_limits<float>::max();

  while(sp > 0) {
    uint32_t nodeIdx = stack[--sp]; 
    const Node& node = nodes_[nodeIdx]; 

    if(dist2ToAABB(p, node.bounds) >= min_dist) {
      continue; 
    }

    if(node.isLeaf()) {
      uint32_t start = node.leafStart();
      uint32_t cnt   = node.leafCount();
      for (uint32_t i = start; i < start + cnt; ++i) {
        uint32_t ti = triIndices_[i];
        const Vector3i& f = faces_[ti];
        Vector3f tmp_p;
        float dist = dist2ToTriangle(p, vertices_, f, tmp_p);
        if(dist < min_dist) {
          min_dist = dist;
          triIndex = ti;
          out_p = tmp_p;
        }
      }
    } else {
      uint32_t left  = node.left;
      uint32_t right = node.right;

      float dist_left = dist2ToAABB(p, nodes_[left].bounds);
      float dist_right = dist2ToAABB(p, nodes_[right].bounds);

      if(dist_left < dist_right)  {
        if(dist_right < min_dist) stack[sp++] = right;
        if(dist_left < min_dist) stack[sp++] = left;
      }
      else {
        if(dist_left < min_dist) stack[sp++] = left;
        if(dist_right < min_dist) stack[sp++] = right;
      }
    }

  }
  return std::sqrt(min_dist);
}

// --- brute-force debug ---

bool BVH::intersectBruteForce(Ray& ray, IntersectionTemporary& data) const {
  if (triCount_ == 0) return false;

  bool hit = false;
  for (uint32_t i = 0; i < triCount_; ++i) {
    const Vector3i& f = faces_[i];
    float t, u, v;
    if (rayTriIntersect(ray, vertices_[f.x()], vertices_[f.y()], vertices_[f.z()], t, u, v)) {
      ray.setFarT(t);
      auto* mi = data.as<MeshIntersection>();
      mi->triIndex = i;
      mi->u = u;
      mi->v = v;
      hit = true;
    }
  }
  return hit;
}

} // namespace mupsi
