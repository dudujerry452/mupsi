#include "gpnoise.h"
#include "math/sampler.h"
#include "math/sample.h"

namespace mupsi {


GPSettings g_gpSettings;

Vector4f SparseGPNoiseGenerator::InternalNoise1D(float t, uint32_t seed) const
{
  float kr = kernel_->getKernelRadius();
  float t_grid = t / kr;
  float frac = t_grid - std::floor(t_grid);
  int i = int(std::floor(t_grid));

  float L2 = kernel_->getLengthScale().squaredNorm();
  float val = 0.f, grad_z = 0.f;

  for (int dx = -1; dx <= 1; dx++) {
    int cell_idx = i + dx;
    uint32_t cell_seed = make_seed(cell_idx, seed);
    Random rng(cell_seed + 1u);

    for (int k = 0; k < impulseDensity_; k++) {
      float t_i = rng.next1D();
      float dt_grid = (frac - float(dx)) - t_i; // distance in grid units
      float dt = dt_grid * kr;                 // world-space distance along ray

      if (dt * dt < kr * kr) {  // within kernel support
        float wi = rng.standard_normal();
        float hv = kernel_->h(Vector3f(0.0f, 0.0f, t_i * kr), Vector3f(0.0f, 0.0f, frac * kr));
        val    += wi * hv;                     // value
        grad_z += wi * hv * (-2.0f / L2) * dt; // ∂/∂t gradient
      }
    }
  }

  // XY gradient: independent N(0, 1/(2·ℓ²)) per component (isotropic single-scale)
  Random rng_xy(make_seed(seed, 0xDEADu));
  float scale = std::sqrt(0.5f) / kernel_->getLengthScale().x();
  float grad_x = rng_xy.standard_normal() * scale;
  float grad_y = rng_xy.standard_normal() * scale;

  return Vector4f(val, grad_x, grad_y, grad_z);
}

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