#ifndef _TRACE_H_
#define _TRACE_H_

#include "../geometry/scene.h"
#include "../geometry/ray.h"

namespace mupsi {

Intersection castRay(const Ray &ray, const SDFScene &scene); 


}

#endif 