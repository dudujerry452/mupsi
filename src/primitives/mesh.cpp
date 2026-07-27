#include "mesh.h"
#include "bsdf/bsdf.h"
#include "texture/texture.h"
#include "math/sampler.h"
#include "math/sample.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <Eigen/Geometry>

namespace mupsi {

// --- OBJ Loading ---

bool Mesh::fetchFrom(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Mesh::fetchFrom: cannot open file " << filename << std::endl;
    return false;
  }

  vertices_.clear();
  normals_.clear();
  texcoords_.clear();
  faces_.clear();

  std::string line;
  while (std::getline(file, line)) {
    // Trim trailing \r (Windows line endings)
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    // Skip empty lines and comments
    if (line.empty() || line[0] == '#')
      continue;

    std::istringstream iss(line);
    std::string type;
    iss >> type;

    if (type == "v") {
      float x, y, z;
      iss >> x >> y >> z;
      vertices_.emplace_back(x, y, z);
    } else if (type == "vn") {
      float x, y, z;
      iss >> x >> y >> z;
      normals_.emplace_back(x, y, z);
    } else if (type == "vt") {
      float u, v, w = 0;
      iss >> u >> v;
      if (iss >> w) {} // consume optional w
      texcoords_.emplace_back(u, v);
    } else if (type == "f") {
      std::vector<uint32_t> vindices;
      std::string vertSpec;
      while (iss >> vertSpec) {
        // Parse v/vt/vn format
        int vi = 0, vti = 0, vni = 0;
        // Count slashes
        size_t slash1 = vertSpec.find('/');
        if (slash1 == std::string::npos) {
          // Format: v
          vi = std::stoi(vertSpec);
        } else {
          vi = std::stoi(vertSpec.substr(0, slash1));
          size_t slash2 = vertSpec.find('/', slash1 + 1);
          if (slash2 == std::string::npos || slash1 + 1 == slash2) {
            // Format: v/ or v/vt or v//vn
            if (slash1 + 1 < vertSpec.size()) {
              if (slash1 + 1 != slash2) {
                std::string mid = vertSpec.substr(slash1 + 1, (slash2 == std::string::npos) ? std::string::npos : slash2 - slash1 - 1);
                if (!mid.empty()) vti = std::stoi(mid);
              }
            }
            if (slash2 != std::string::npos && slash2 + 1 < vertSpec.size()) {
              vni = std::stoi(vertSpec.substr(slash2 + 1));
            }
          } else {
            // Format: v/vt/vn
            vti = std::stoi(vertSpec.substr(slash1 + 1, slash2 - slash1 - 1));
            vni = std::stoi(vertSpec.substr(slash2 + 1));
          }
        }

        // Normalize negative indices (OBJ uses 1-based, negative = relative to end)
        auto normIdx = [](int idx, size_t count) -> uint32_t {
          if (idx > 0) return static_cast<uint32_t>(idx - 1);
          if (idx < 0) return static_cast<uint32_t>(static_cast<int>(count) + idx);
          return 0; // should not happen
        };

        vindices.push_back(normIdx(vi, vertices_.size()));
        // Store texcoord and normal indices for later (unused for now,
        // but we consume them to avoid parsing errors)
        (void)vti;
        (void)vni;
      }

      if (vindices.size() < 3) {
        std::cerr << "Mesh::fetchFrom: face with < 3 vertices, skipping" << std::endl;
        continue;
      }
      if (vindices.size() > 3) {
        std::cerr << "Mesh::fetchFrom: face with " << vindices.size()
                  << " vertices, skipping (only triangles supported)" << std::endl;
        continue;
      }

      faces_.emplace_back(vindices[0], vindices[1], vindices[2]);
    }
    // Ignore other line types (o, g, s, usemtl, mtllib, etc.)
  }

  file.close();

  std::cout << "Mesh::fetchFrom: loaded " << filename
            << " (" << vertices_.size() << " vertices, "
            << faces_.size() << " faces)" << std::endl;

  if (vertices_.empty() || faces_.empty()) {
    std::cerr << "Mesh::fetchFrom: no valid geometry loaded" << std::endl;
    return false;
  }

  // Compute per-face geometric normals
  computeFaceNormals();

  // Initialize BSDF with default
  bsdf_ = Primitive::default_bsdf_;

  return true;
}

// --- Helper methods ---

void Mesh::computeFaceNormals() {
  faceNormals_.resize(faces_.size());
  for (size_t i = 0; i < faces_.size(); ++i) {
    const Vector3i& f = faces_[i];
    Vector3f edge1 = vertices_[f.y()] - vertices_[f.x()];
    Vector3f edge2 = vertices_[f.z()] - vertices_[f.x()];
    Vector3f n = edge1.cross(edge2);
    float len = n.norm();
    if (len > 1e-12f) {
      faceNormals_[i] = n / len;
    } else {
      faceNormals_[i] = Vector3f::Zero(); // degenerate
    }
  }
}

float Mesh::computeFaceArea(uint32_t faceIdx) const {
  const Vector3i& f = faces_[faceIdx];
  Vector3f edge1 = vertices_[f.y()] - vertices_[f.x()];
  Vector3f edge2 = vertices_[f.z()] - vertices_[f.x()];
  return 0.5f * edge1.cross(edge2).norm();
}

Vector3f Mesh::interpolateNormal(uint32_t faceIdx, float u, float v) const {
  if (normals_.empty()) {
    return faceNormals_[faceIdx];
  }
  const Vector3i& f = faces_[faceIdx];
  float w = 1.0f - u - v;
  Vector3f n = w * normals_[f.x()] + u * normals_[f.y()] + v * normals_[f.z()];
  float len = n.norm();
  return len > 1e-12f ? n / len : faceNormals_[faceIdx];
}

Vector2f Mesh::interpolateTexcoord(uint32_t faceIdx, float u, float v) const {
  if (texcoords_.empty()) {
    return Vector2f(u, v);
  }
  const Vector3i& f = faces_[faceIdx];
  float w = 1.0f - u - v;
  return w * texcoords_[f.x()] + u * texcoords_[f.y()] + v * texcoords_[f.z()];
}

// --- Primitive interface ---

bool Mesh::intersect(Ray& ray, IntersectionTemporary& data) const {
  bool hit = bvh_.intersect(ray, data);
  if (hit) {
    data.primitive = this;
  }
  return hit;
}

void Mesh::intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const {
  auto* mi = data.as<MeshIntersection>();
  uint32_t fi = mi->triIndex;
  float u = mi->u;
  float v = mi->v;

  info.primitive = this;
  info.t = info.t;
  info.p = info.p;
  info.Ng = interpolateNormal(fi, u, v);
  info.uv = interpolateTexcoord(fi, u, v);
  info.bsdf = getBsdf(0);
}

bool Mesh::occluded(const Ray& ray) const {
  return bvh_.occluded(ray);
}

bool Mesh::sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const {
  if (faceCdf_.empty() || totalArea_ <= 0.0f || faces_.empty())
    return false;

  // Binary search on CDF to pick a face
  float r = sampler.next1D();
  auto it = std::lower_bound(faceCdf_.begin(), faceCdf_.end(), r);
  uint32_t faceIdx = static_cast<uint32_t>(it - faceCdf_.begin());
  if (faceIdx >= faces_.size())
    faceIdx = static_cast<uint32_t>(faces_.size()) - 1;

  // Sample random point on triangle via barycentric coordinates
  float r1 = sampler.next1D();
  float r2 = sampler.next1D();
  float sqrt_r1 = std::sqrt(r1);
  float bary_u = 1.0f - sqrt_r1;
  float bary_v = r2 * sqrt_r1;

  const Vector3i& f = faces_[faceIdx];
  const Vector3f& v0 = vertices_[f.x()];
  const Vector3f& v1 = vertices_[f.y()];
  const Vector3f& v2 = vertices_[f.z()];
  Vector3f edge1 = v1 - v0;
  Vector3f edge2 = v2 - v0;
  Vector3f pt = v0 + edge1 * bary_u + edge2 * bary_v;

  Vector3f dir = pt - p;
  float dist2 = dir.squaredNorm();
  float dist = std::sqrt(dist2);
  sample.d = dir / dist;
  sample.dist = dist;
  sample.pdf = 1.0f / totalArea_;

  Vector2f uv = interpolateTexcoord(faceIdx, bary_u, bary_v);
  sample.weight = (emission_ ? (*emission_)[uv] : Vector3f::Zero())
                / std::max(sample.pdf, 1e-6f);

  return true;
}

void Mesh::prepareForRender() {
  // Ensure face normals are computed
  if (faceNormals_.empty()) {
    computeFaceNormals();
  }

  // Build BVH
  bvh_.build(vertices_.data(), faces_.data(), static_cast<uint32_t>(faces_.size()));

  // Compute face areas and CDF for light sampling
  totalArea_ = 0.0f;
  faceCdf_.resize(faces_.size());
  for (size_t i = 0; i < faces_.size(); ++i) {
    float area = computeFaceArea(static_cast<uint32_t>(i));
    totalArea_ += area;
    faceCdf_[i] = totalArea_;
  }
  if (totalArea_ > 0.0f) {
    for (auto& c : faceCdf_) c /= totalArea_;
  }
}

const Bsdf* Mesh::getBsdf(int /*index*/) const {
  return bsdf_.get();
}

} // namespace mupsi
