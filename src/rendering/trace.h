#ifndef _TRACE_H_
#define _TRACE_H_

#include "geometry/scene.h"
#include "geometry/ray.h"
#include "bsdf/bsdf.h"
#include "geometry/intersection.h"
#include "math/random.h"
#include <cstdint>
#include <memory>

namespace mupsi {

struct RayTraceConfig {
  float dt = 0.1;
  float depth = 50;
  float eps = 0.01;
  int binarynum = 3;
  int max_bounce = 3;
};

enum class GPCorrelationMode { SingleRealization, RenewalPlus };
extern GPCorrelationMode g_gpMode;

extern RayTraceConfig g_rayTraceConfig;

extern thread_local uint32_t g_pixel_x;
extern thread_local uint32_t g_pixel_y;
extern thread_local uint32_t g_spp;

uint32_t computeGPSeed(uint32_t base_seed, uint32_t bounce);

Intersection castRay(const Ray &ray, SDFScene &scene);
Vector3f traceRay(const Ray &ray, SDFScene &scene, int depth);

struct PathTracerSettings {
  int spp = 1; 
  int max_bounce = 3; 
  int max_medium_bounce = 3; 
  float eps = 1e-4; 
  float rr = 0.5; 
};


class PathTracer {

  std::shared_ptr<PathTracerSettings> settings_; 

public: 
  PathTracer() = default; 
  PathTracer(std::shared_ptr<PathTracerSettings> settings): settings_(settings) {}
  virtual ~PathTracer() = default; 

  SurfaceScatterEvent makeSurfaceScatterEvent(IntersectionTemporary& data, IntersectionInfo& info, Ray& ray, UniformPathSampler* sampler); 

  bool handleSurface(SurfaceScatterEvent& event, Vector3f& throughput, Vector3f& emission, 
    Ray& ray, 
    IntersectionTemporary& data, 
    IntersectionInfo& info, 
    Scene& scene, 
    Sampler* sampler);
    
  Vector3f trace(Vector2i pixel, Scene& scene, uint32_t seed, int spp); 
}; 
}

#endif 