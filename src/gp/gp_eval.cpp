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
        Random rng(seed);

        Vector3f ngbf = neighbor.cast<float>() * cell_size;

        for(int i = 0; i < points_per_cell; i++) {

          Vector3f offset = Vector3f(rng.uniform(), rng.uniform(), rng.uniform()); 
          if (offset.norm() > 1.0) continue; // only sample points inside the unit sphere
          offset = offset * cell_size;
          Vector3f sample_point = ngbf + offset;
          
          sum += rng.standard_normal() * kernel.h(sample_point, pos);
        }
      }
    }
  }
  
  return sum;
}

} // namespace mupsi