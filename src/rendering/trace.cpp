#include "trace.h"
#include "math/random.h"
#include "camera.h"
#include "geometry/intersection.h"
#include "bsdf/bsdf.h"
#include "geometry/primitive.h"

#include <Eigen/Geometry>
#include <iostream>

using namespace Eigen;

namespace mupsi {

// note: cast Ray can trace both SDFScene and GPScene, GPScene's eval is overriden.

RayTraceConfig g_rayTraceConfig;
GPCorrelationMode g_gpMode = GPCorrelationMode::SingleRealization;

thread_local uint32_t g_pixel_x = 0;
thread_local uint32_t g_pixel_y = 0;
thread_local uint32_t g_spp = 0;

uint32_t computeGPSeed(uint32_t base_seed, uint32_t bounce) {
    if (g_gpMode == GPCorrelationMode::SingleRealization)
        return base_seed;
    return base_seed + xxhash32(g_pixel_x, g_pixel_y, g_spp, bounce);
}

Intersection castRay(const Ray &ray, SDFScene &scene)
{
  float t = 0.0;
  bool hit = false;
  const float &eps = g_rayTraceConfig.eps, &dt = g_rayTraceConfig.dt;
  const int &depth = g_rayTraceConfig.depth, &binarynum = g_rayTraceConfig.binarynum;

  const SDFObject* obj = nullptr;
  if (scene.eval(ray.origin(), obj) < eps) // start point is inside the object
    ; // std::cout << "inside !" << std::endl ;                                               // not sure how to deal with it
  else
    for (int i = 0; i < depth; i++)
    {
      t += dt;
      Vector3f pos = ray.origin() +
                     ray.direction() * t;
      float v = scene.eval(pos, obj);

      if (v < eps)
      {
        float l = -dt, r = 0, mid = -dt / 2;
        for (int j = 0; j < binarynum; j++)
        {
          float midv = scene.eval(ray.origin() + ray.direction() * (t + mid), obj);
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
    Vector3f hitPos = ray.origin() + ray.direction() * t;
    normal = scene.getNormal(hitPos);
    material = obj->getMaterial();
  }

  return Intersection{hit, t, ray.origin() + ray.direction() * t, normal, material}; // Placeholder normal, replace with actual normal calculation
}

Vector3f traceRay(const Ray &ray, SDFScene &scene, int depth) {

  if(depth > g_rayTraceConfig.max_bounce)
    return Vector3f::Zero();

  auto* gpScene = dynamic_cast<GPScene*>(&scene);

  // Set per-bounce seed (thread_local — no data race across OpenMP threads)
  if (gpScene) {
    uint32_t base_seed = gpScene->getGPNoise().getSeed();
    g_cond_seed = computeGPSeed(base_seed, depth);
  }

  Intersection its = castRay(ray, scene);
  if (its.hit) {

    Vector3f& N = its.normal;
    const Vector3f& wo = -ray.direction();

    // Prepare conditioning for next bounce (stored in cond_next_, doesn't affect current cond_)
    if (gpScene && g_gpMode == GPCorrelationMode::RenewalPlus) {
      uint32_t base_seed = gpScene->getGPNoise().getSeed();
      uint32_t nextSeed = computeGPSeed(base_seed, depth + 1);
      gpScene->prepareConditioning(its.position, its.normal, nextSeed);
    }

    // light contribute

    std::shared_ptr<Material> mate = its.material;
    Vector3f L_contrib = Vector3f::Zero();

    for (const auto& plight: scene.parallel_lights) {
      Vector3f ws = -plight.direction.normalized();
      float cos1 = ws.dot(N);
      float cos2 = (-ws).dot(-ws);

      if(cos1 > 0.0f && cos2 > 0.0f) {
        // 100 * g_rayTraceConfig.dt: a expierenece value to avoid 阴影摩尔纹
        Ray pray = Ray(its.position + N * g_rayTraceConfig.eps * 100 * g_rayTraceConfig.dt, ws);  // TODO: for some reason, *100 should be good to avoid self intersection, but not sure why.

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

      Vector3f L_rev;

      if (gpScene && g_gpMode == GPCorrelationMode::RenewalPlus) {
        auto [oldCond, oldSeed] = gpScene->activateNextConditioning();
        L_rev = traceRay(
          Ray(
              its.position + N * g_rayTraceConfig.eps*100 * g_rayTraceConfig.dt,
              wi
            ), scene, depth+1
          );
        gpScene->restoreConditioning(oldCond, oldSeed);
      } else {
        L_rev = traceRay(
          Ray(
              its.position + N * g_rayTraceConfig.eps*100 * g_rayTraceConfig.dt,
              wi
            ), scene, depth+1
          );
      }

      L_ind = L_rev.cwiseProduct(mate->evalRadiance(wo, wi, N)) * std::fabs(wi.dot(N)) / pdf; // / RR

    }

    Vector3f sum = L_contrib + L_ind;
    return sum;
  }

  return Vector3f::Zero();
}

SurfaceScatterEvent PathTracer::makeSurfaceScatterEvent(IntersectionTemporary& data, IntersectionInfo& info, Ray& ray, UniformPathSampler* sampler) {
  SurfaceScatterEvent event;
  event.wo = -ray.direction();
  event.normal = info.Ng;
  event.sampler = sampler;

  return event;
}

Vector3f PathTracer::trace(Vector2i pixel, Scene& scene, uint32_t seed, int spp) {
  auto pix_seed = make_seed(pixel.x(), pixel.y(), spp, seed);
  UniformPathSampler path_sampler(pix_seed);

  Ray ray = scene.cam().generateRay(pixel.x(), pixel.y());

  Vector3f emission = Vector3f::Zero();
  Vector3f throughput = Vector3f::Ones() ; 
  bool hasHit = false; 
  int bounce_times = 0; 
  do {
    IntersectionTemporary data; 
    IntersectionInfo info;
    scene.intersect(ray, data, info);
    if(data.primitive) {
      hasHit = true;
      SurfaceScatterEvent event = makeSurfaceScatterEvent(data, info, ray, &path_sampler);

      bool backside = event.wo.dot(event.normal) < 0;
      if(backside) {
        event.normal = -event.normal;
      } 

      // light sample 
      
    

      // bsdf sample 
      // if(!backside && data.primitive->hasEmission()) {
      //   emission += throughput.cwiseProduct(data.primitive->getEmissionIntensity()); 
      // }

      info.bsdf->sample(event); // 填充wi, pdf, rad

      throughput = throughput.cwiseProduct((event.rad));
      float survival = throughput.maxCoeff();

      if (path_sampler.next1D() > survival) 
        break; 
      throughput /= survival;




      ray = Ray(info.p + info.Ng * settings_->eps, event.wi);
      bounce_times ++; 

    } else {
      hasHit = false; 
    }
  }while(hasHit && bounce_times < settings_->max_bounce); 

  return throughput;
}
}