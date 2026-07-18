#include "trace.h"
#include "gp/gp_eval.h"
#include "math/random.h"

#include <Eigen/Geometry> 
#include <iostream> 

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

  const SDFObject* obj = nullptr;
  if (scene.eval(ray.getOrigin(), obj) < eps) // start point is inside the object
    ; // std::cout << "inside !" << std::endl ;                                               // not sure how to deal with it
  else
    for (int i = 0; i < depth; i++)
    {
      t += dt;
      Vector3f pos = ray.getOrigin() +
                     ray.getDirection() * t;
      float v = scene.eval(pos, obj);

      if (v < eps)
      {
        float l = -dt, r = 0, mid = -dt / 2;
        for (int j = 0; j < binarynum; j++)
        {
          float midv = scene.eval(ray.getOrigin() + ray.getDirection() * (t + mid), obj);
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
  std::shared_ptr<Material> material = nullptr; 
  if (hit)
  {
    // calculate normal with difference
    Vector3f hitPos = ray.getOrigin() + ray.getDirection() * t;
    normal = scene.getNormal(hitPos);
    material = obj->getMaterial();
  }

  return Intersection{hit, t, ray.getOrigin() + ray.getDirection() * t, normal, material}; // Placeholder normal, replace with actual normal calculation
}

Vector3f traceRay(const Ray &ray, const SDFScene &scene, int depth) {

  if(depth > g_rayTraceConfig.depth) 
    return Vector3f::Zero();

  Intersection its = castRay(ray, scene);
  if (its.hit) {

    Vector3f& N = its.normal; 
    const Vector3f& wo = -ray.getDirection();

    // light contribute 

    std::shared_ptr<Material> mate = its.material;
    Vector3f L_contrib = Vector3f::Zero();

    for (const auto& plight: scene.parallel_lights) {
      Vector3f ws = -plight.direction.normalized();  
      float cos1 = ws.dot(N); 
      float cos2 = (-ws).dot(-ws); 

      if(cos1 > 0.0f && cos2 > 0.0f) {
        Ray pray = Ray(its.position + N * g_rayTraceConfig.eps * 100, ws);  // TODO: for some reason, *100 should be good to avoid self intersection, but not sure why.

        Intersection its2 = castRay(pray, scene);
        if(!its2.hit) {

          Vector3f single = plight.intensity.cwiseProduct(mate->evalRadiance(wo, ws, N)) * cos1 * cos2;  // / pdf / light attnuention
          L_contrib += single;
        } 
      }
    }

    // indirect contribute 

    Vector3f L_ind = Vector3f::Zero(); 

    Vector3f wi; float pdf; mate->bsdf(wo, N, wi, pdf);

    if (wo.dot(N) > 0.0f) { // wo . N > 0.0f 
      Vector3f L_rev = traceRay(
      Ray(
          its.position + N * g_rayTraceConfig.eps * 100, 
          wi
        ), scene, depth+1
      ); 

      L_ind = L_rev.cwiseProduct(mate->evalRadiance(wo, wi, N)) * std::fabs(wi.dot(N)) / pdf; // / RR

    }

    Vector3f sum = L_contrib + L_ind;
    return sum; 
  }

  return Vector3f::Zero();
} 

}