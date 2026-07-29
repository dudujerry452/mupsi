#include "medium.h"
#include "math/sampler.h"

namespace mupsi {

  // default_bsdf_ lives in bsdf.cpp to guarantee init after Bsdf::default_albedo_

  SurfaceScatterEvent Medium::makeSurfaceEventFromMedium(const MediumSample& sample, 
        IntersectionInfo& info, Ray& ray, Sampler& sampler, Sampler& medium_sampler) const {
      SurfaceScatterEvent event; 
      event.wo = -ray.direction(); // toward previous vertex, matches hard surface convention
      event.normal = sample.normal; 
      event.sampler = &sampler;

      info.bsdf = getBsdf().get();
      info.p = sample.p;  
      info.t = sample.t;
      info.Ng = sample.normal;
      info.uv = {0.0f, 0.0f};
      info.primitive = nullptr; 

      info.sceneSeed = medium_sampler.getSeed();  // it is seed 
      
      event.info = &info; 
      return event; 
    }
}