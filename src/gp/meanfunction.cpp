#include "meanfunction.h"
#include <Eigen/Core> 

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
  
} // namespace mupsi 
