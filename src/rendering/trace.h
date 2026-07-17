#ifndef _TRACE_H_
#define _TRACE_H_

#include "geometry/scene.h"
#include "geometry/ray.h"

namespace mupsi {

struct RayTraceConfig {
  float dt = 0.1; 
  float depth = 50;  
  float eps = 0.01; 
  int binarynum = 3;
};

extern RayTraceConfig g_rayTraceConfig;

Intersection castRay(const Ray &ray, const SDFScene &scene); 


}

#endif 