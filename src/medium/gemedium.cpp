#include "gpmedium.h"
#include "geometry/ray.h"

namespace mupsi {

  void GPMedium::sampleDistance(Ray& ray, MediumSample& sample, Sampler& sampler) const
  {

    float nt = ray.nearT(), ft = ray.farT();
    ft = std::min(ft, 2000.0f); 

    

  }
  

}