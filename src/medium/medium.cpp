#include "medium.h"
#include "math/sampler.h"

namespace mupsi {

  std::shared_ptr<Bsdf> Medium::default_bsdf_ = std::make_shared<LambertianBsdf>();


  SurfaceScatterEvent Medium::makeSurfaceEventFromMedium(const MediumSample& sample, 
        IntersectionInfo& info, Ray& ray, Sampler& sampler, Sampler& medium_sampler) const {
      SurfaceScatterEvent event; 
      event.wo = ray.direction(); 
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