#ifndef _TRACE_H_
#define _TRACE_H_

#include "geometry/scene.h"
#include "geometry/ray.h"
#include <cstdint>

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


}

#endif 