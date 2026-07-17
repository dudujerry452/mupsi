#include "trace.h"

using namespace Eigen; 

namespace mupsi {

// note: cast Ray can trace both SDFScene and GPScene, GPScene's eval is overriden. 

RayTraceConfig g_rayTraceConfig;

Intersection castRay(const Ray &ray, const SDFScene &scene)
{
  float t = 0.0;
  bool hit = false;
  const float &eps = g_rayTraceConfig.eps, &dt = g_rayTraceConfig.dt;
  const int &depth = g_rayTraceConfig.depth, &binarynum = g_rayTraceConfig.binarynum;

  if (scene.eval(ray.getOrigin()) < eps) // start point is inside the object
    ;                                               // not sure how to deal with it
  else
    for (int i = 0; i < depth; i++)
    {
      t += dt;
      Vector3f pos = ray.getOrigin() +
                     ray.getDirection() * t;
      float v = scene.eval(pos);

      if (v < eps)
      {
        float l = -dt, r = 0, mid = -dt / 2;
        for (int j = 0; j < binarynum; j++)
        {
          float midv = scene.eval(ray.getOrigin() + ray.getDirection() * (t + mid));
          if (midv < eps)
            r = mid;
          else
            l = mid;
          mid = (l+r)/2; 
        }
        hit = true;
        break;
      }
    }

  Vector3f normal = Vector3f::Zero();
  if (hit)
  {
    // calculate normal with difference
    Vector3f hitPos = ray.getOrigin() + ray.getDirection() * t;
    normal =
        {
            scene.eval(hitPos + Vector3f{eps, 0, 0}) - scene.eval(hitPos - Vector3f{eps, 0, 0}),
            scene.eval(hitPos + Vector3f{0, eps, 0}) - scene.eval(hitPos - Vector3f{0, eps, 0}),
            scene.eval(hitPos + Vector3f{0, 0, eps}) - scene.eval(hitPos - Vector3f{0, 0, eps})};
    normal.normalize();
  }

  return Intersection{hit, t, ray.getOrigin() + ray.getDirection() * t, normal}; // Placeholder normal, replace with actual normal calculation
}

}