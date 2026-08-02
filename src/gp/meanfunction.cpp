#include "meanfunction.h"
#include <Eigen/Core>
#include <cstdio>
#include <cmath>
#include <iostream>

using namespace Eigen; 

namespace mupsi 
{


  float SphereMeanFunction::eval(const Vector3f& p) const
  {
    return (p - center_).norm() - radius_;
  }
  Vector3f SphereMeanFunction::gradient(const Vector3f& p) const {
    Vector3f diff = p - center_;
    return diff.normalized();
  }

  float MeshMeanFunction::eval(const Vector3f& p) const {
    float inside = mesh_->inside(p) ? -1.0f : 1.0f;
    uint32_t tri; Vector3f out_p;
    return inside * mesh_->distToPoint(p, tri, out_p);
  }
  Vector3f MeshMeanFunction::gradient(const Vector3f& p) const {
    float inside = mesh_->inside(p) ? -1.0f : 1.0f;
    uint32_t tri; Vector3f out_p;
    float d = mesh_->distToPoint(p, tri, out_p);
    if(d < 1e-6f) {
      Vector3f n = mesh_->faceNormals_[tri];           // local-space
      if (!mesh_->normalTransform_.isIdentity()) {
        n = (mesh_->normalTransform_ * n).normalized();
      }
      return n; // faceNormal already points outward
    }
    return (inside * (p - out_p)).normalized();
  }
  
GridMeanFunction::GridMeanFunction(const std::string& filepath, int res,
                                     const Vector3f& bounds_min, const Vector3f& bounds_max)
    : res_(res), bounds_min_(bounds_min), bounds_max_(bounds_max) {
  inv_cell_size_ = Vector3f(res, res, res).cwiseQuotient(bounds_max_ - bounds_min_);
  size_t total = res * res * res;
  data_.resize(total);
  FILE* fp = fopen(filepath.c_str(), "rb");
  if (!fp) {
    std::cerr << "GridMeanFunction: cannot open " << filepath << std::endl;
    return;
  }
  fread(data_.data(), sizeof(float), total, fp);
  fclose(fp);
}

float GridMeanFunction::eval(const Vector3f& p) const {
  if (data_.empty()) return 0.0f;
  Vector3f frac = (p - bounds_min_).cwiseProduct(inv_cell_size_) - Vector3f(0.5f, 0.5f, 0.5f);
  int i0 = std::max(0, std::min(res_ - 2, (int)std::floor(frac.x())));
  int j0 = std::max(0, std::min(res_ - 2, (int)std::floor(frac.y())));
  int k0 = std::max(0, std::min(res_ - 2, (int)std::floor(frac.z())));
  int i1 = i0 + 1, j1 = j0 + 1, k1 = k0 + 1;
  // clamp weights to [0,1] to prevent extrapolation outside the grid
  float wx = std::clamp(frac.x() - i0, 0.0f, 1.0f);
  float wy = std::clamp(frac.y() - j0, 0.0f, 1.0f);
  float wz = std::clamp(frac.z() - k0, 0.0f, 1.0f);

  int stride_y = res_, stride_x = res_ * res_;
  auto at = [&](int i, int j, int k) -> float { return data_[i * stride_x + j * stride_y + k]; };

  float c00 = at(i0, j0, k0) * (1 - wx) + at(i1, j0, k0) * wx;
  float c01 = at(i0, j0, k1) * (1 - wx) + at(i1, j0, k1) * wx;
  float c10 = at(i0, j1, k0) * (1 - wx) + at(i1, j1, k0) * wx;
  float c11 = at(i0, j1, k1) * (1 - wx) + at(i1, j1, k1) * wx;
  float c0  = c00 * (1 - wy) + c10 * wy;
  float c1  = c01 * (1 - wy) + c11 * wy;
  return c0 * (1 - wz) + c1 * wz;
}

Vector3f GridMeanFunction::gradient(const Vector3f& p) const {
  float eps = 1e-4f;
  float dx = eval(Vector3f(p.x() + eps, p.y(), p.z())) - eval(Vector3f(p.x() - eps, p.y(), p.z()));
  float dy = eval(Vector3f(p.x(), p.y() + eps, p.z())) - eval(Vector3f(p.x(), p.y() - eps, p.z()));
  float dz = eval(Vector3f(p.x(), p.y(), p.z() + eps)) - eval(Vector3f(p.x(), p.y(), p.z() - eps));
  return Vector3f(dx, dy, dz) / (2.0f * eps);
}

} // namespace mupsi 
