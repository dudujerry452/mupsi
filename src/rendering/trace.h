#ifndef _TRACE_H_
#define _TRACE_H_

#include "geometry/scene.h"
#include "geometry/ray.h"
#include "bsdf/bsdf.h"
#include "geometry/intersection.h"
#include "math/sampler.h"
#include <cstdint>
#include <memory>

namespace mupsi {

struct PathTracerSettings {
  int spp = 1; 
  int max_bounce = 3; 
  int max_medium_bounce = 3; 
  float eps = 1e-4; 
  float rr = 0.5; 
};

class Medium; 
class MediumSample;


class PathTracer {

  static std::shared_ptr<PathTracerSettings> settings_; 

public:
  PathTracer() = default;
  virtual ~PathTracer() = default;

  static PathTracerSettings& settings() { return *settings_; }

  SurfaceScatterEvent makeSurfaceScatterEvent(IntersectionTemporary& data, IntersectionInfo& info, Ray& ray, UniformPathSampler& sampler); 

  bool handleSurface(SurfaceScatterEvent& event, Vector3f& throughput, Vector3f& emission, 
    Ray& ray, 
    Scene& scene, 
    Sampler& sampler);

  bool handleVolume(SurfaceScatterEvent& event, 
    MediumSample& sample,
    Medium& medium,
    Vector3f& throughput, Vector3f& emission, 
    Ray& ray, 
    Scene& scene, 
    Sampler& sampler, 
    Sampler& medium_sampler
  );
    
  Vector3f trace(Vector2i pixel, Scene& scene, uint32_t seed, int spp); 
}; 
}

#endif 