#include "gpnoise.h"
#include "math/sampler.h"
#include "math/sample.h"

namespace mupsi {


GPSettings g_gpSettings;

Vector4f SparseGPNoiseGenerator::InternalNoise(const Vector3f& p, uint32_t seed) const
{

  Vector3i cell = (p/kernel_->getKernelRadius()).cast<int>();
  Vector4f sum = Vector4f::Zero();
  for(int dx = -1; dx <= 1; dx++) {
    for(int dy = -1; dy <= 1; dy++) {
      for(int dz = -1; dz <= 1; dz++) {
        Vector3i neighbor = cell + Vector3i(dx, dy, dz);
        uint32_t cell_seed = make_seed(neighbor.x(), neighbor.y(), neighbor.z(), seed);
        Random rng(cell_seed + 1u);  // +1 same as sparse-gpis

        Vector3f ngbf = neighbor.cast<float>() * kernel_->getKernelRadius();

        for(int i = 0; i < impulseDensity_; i++) {

          Vector3f offset = Vector3f(rng.next1D(), rng.next1D(), rng.next1D());
          // if (offset.squaredNorm() > 1.0f) continue;
          Vector3f sample_point = ngbf + offset * kernel_->getKernelRadius();

          float wi = rng.standard_normal();
          sum.x() += wi * kernel_->h(sample_point, p); // sum_i wi * h(s, p)

          // package grad to sum
          Vector3f grad = kernel_->h_grad(sample_point, p);
          sum.y() += wi * grad.x();
          sum.z() += wi * grad.y();
          sum.w() += wi * grad.z();
        }
      }
    }
  }
  return sum;
}

} // namespace mupsi