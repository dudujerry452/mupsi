#include "gp_eval.h"
#include "../math/random.h"

namespace mupsi {

float gp_eval(const Vector3f& pos, const SEKernel& kernel, int points_per_cell, float cell_size, uint32_t g_seed) {

  Vector3i cell = (pos/cell_size).cast<int>();
  float sum = 0; 
  for(int dx = -1; dx <= 1; dx++) {
    for(int dy = -1; dy <= 1; dy++) {
      for(int dz = -1; dz <= 1; dz++) {
        Vector3i neighbor = cell + Vector3i(dx, dy, dz);
        uint32_t seed = make_seed(neighbor.x(), neighbor.y(), neighbor.z(), g_seed);
        Random rng(seed + 1u);  // +1 same as sparse-gpis

        Vector3f ngbf = neighbor.cast<float>() * cell_size;

        for(int i = 0; i < points_per_cell; i++) {

          Vector3f offset = Vector3f(rng.next1D(), rng.next1D(), rng.next1D());
          // if (offset.squaredNorm() > 1.0f) continue;
          Vector3f sample_point = ngbf + offset * cell_size;

          sum += rng.standard_normal() * kernel.h(sample_point, pos);
        }
      }
    }
  }
  // Vector3i cell = (pos/cell_size).cast<int>();
  // Random rng(make_seed(cell.x(), cell.y(), cell.z(), g_seed));  
  // Vector3f sample_point(0.0f, 0.0f, -2.0f); 
  // float sum = rng.standard_normal() * kernel.h(sample_point, pos);
  
  return sum;
}

} // namespace mupsi