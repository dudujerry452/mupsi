#include "trace.h"

using namespace Eigen; 

namespace mupsi {

// note: cast Ray can trace both SDFScene and GPScene, GPScene's eval is overriden. 

Intersection castRay(const Ray &ray, const SDFScene &scene)
{
  float t = 0.0;
  bool hit = false;
  static const float eps = 0.01, dt = 1.0f;
  static const int depth = 500, binarynum = 3;

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