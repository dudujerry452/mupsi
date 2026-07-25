#include "gpnoise.h"
#include "math/sampler.h"
#include "math/sample.h"

namespace mupsi {


float GPNoiseGenerator::RawNoise(const Vector3f& pos, uint32_t seed) const
{

  Vector3i cell = (pos/kernel.getKernelRadius()).cast<int>();
  float sum = 0;
  for(int dx = -1; dx <= 1; dx++) {
    for(int dy = -1; dy <= 1; dy++) {
      for(int dz = -1; dz <= 1; dz++) {
        Vector3i neighbor = cell + Vector3i(dx, dy, dz);
        uint32_t cell_seed = make_seed(neighbor.x(), neighbor.y(), neighbor.z(), seed);
        Random rng(cell_seed + 1u);  // +1 same as sparse-gpis

        Vector3f ngbf = neighbor.cast<float>() * kernel.getKernelRadius();

        for(int i = 0; i < impulseDensity; i++) {

          Vector3f offset = Vector3f(rng.next1D(), rng.next1D(), rng.next1D());
          // if (offset.squaredNorm() > 1.0f) continue;
          Vector3f sample_point = ngbf + offset * kernel.getKernelRadius();

          sum += rng.standard_normal() * kernel.h(sample_point, pos); // sum_i wi * h(s, p)
        }
      }
    }
  }
  return sum;
}



float SparseGPNoiseGenerator::InternalNoise(const Vector3f& p) const
{

  Vector3i cell = (p/kernel_->getKernelRadius()).cast<int>();
  float sum = 0;
  for(int dx = -1; dx <= 1; dx++) {
    for(int dy = -1; dy <= 1; dy++) {
      for(int dz = -1; dz <= 1; dz++) {
        Vector3i neighbor = cell + Vector3i(dx, dy, dz);
        uint32_t cell_seed = make_seed(neighbor.x(), neighbor.y(), neighbor.z(), seed_);
        Random rng(cell_seed + 1u);  // +1 same as sparse-gpis

        Vector3f ngbf = neighbor.cast<float>() * kernel_->getKernelRadius();

        for(int i = 0; i < impulseDensity_; i++) {

          Vector3f offset = Vector3f(rng.next1D(), rng.next1D(), rng.next1D());
          // if (offset.squaredNorm() > 1.0f) continue;
          Vector3f sample_point = ngbf + offset * kernel_->getKernelRadius();

          sum += rng.standard_normal() * kernel_->h(sample_point, p); // sum_i wi * h(s, p)
        }
      }
    }
  }
  return sum;
}

Vector3f SparseGPNoiseGenerator::Gradient(const Vector3f& p) const
{
  // reference: 
  /*
      uint32_t seed = sampler.nextI();
    float eps = gpeps;  // TODO: hard-coded value
    Vector3f normal;
    normal.x() = eval(p + Vector3f(eps, 0, 0), seed) - eval(p - Vector3f(eps, 0, 0), seed);
    normal.y() = eval(p + Vector3f(0, eps, 0), seed) - eval(p - Vector3f(0, eps, 0), seed);
    normal.z() = eval(p + Vector3f(0, 0, eps), seed) - eval(p - Vector3f(0, 0, eps), seed);
    return normal.normalized();

  */

  Vector3f grad; 
  grad.x() = RawNoise(p + Vector3f(gpeps, 0, 0)) - RawNoise(p - Vector3f(gpeps, 0, 0));
  grad.y() = RawNoise(p + Vector3f(0, gpeps, 0)) - RawNoise(p - Vector3f(0, gpeps, 0));
  grad.z() = RawNoise(p + Vector3f(0, 0, gpeps)) - RawNoise(p - Vector3f(0, 0, gpeps));
  return grad.normalized();
}

} // namespace mupsi