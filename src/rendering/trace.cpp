#include "trace.h"
#include "math/sampler.h"
#include "math/random.h"
#include "camera.h"
#include "geometry/intersection.h"
#include "bsdf/bsdf.h"
#include "geometry/primitive.h"
#include "texture/texture.h"
#include "medium/medium.h"
#include "medium/gpmedium.h"

#include <Eigen/Geometry>
#include <iostream>

using namespace Eigen;

namespace mupsi {

PathTracerSettings g_pathTracerSettings; 

SurfaceScatterEvent PathTracer::makeSurfaceScatterEvent(IntersectionInfo& info, Ray& ray, UniformPathSampler& sampler) {
  SurfaceScatterEvent event;
  event.wo = -ray.direction();
  event.normal = info.Ng;
  event.sampler = &sampler;
  event.info = &info; 

  return event;
}

bool PathTracer::handleSurface(SurfaceScatterEvent& event, Vector3f& throughput, Vector3f& emission, 
    Ray& ray, 
    Scene& scene, 
    Sampler& sampler) {

    const IntersectionInfo& info = *event.info;

    // light sample
    LightSample light_sample;
    if(scene.chooseLight(info.p, sampler, light_sample)) {
      Ray ray(info.p + event.normal * g_pathTracerSettings.eps, light_sample.d);
      float bias_dist = light_sample.dist - g_pathTracerSettings.eps * std::abs(light_sample.d.dot(event.normal));
      ray.setFarT(bias_dist * 0.99f);
      if(!scene.occluded(ray)) {
        // Evaluate BSDF at light direction to get albedo for direct lighting.

        SurfaceScatterEvent directEvent = event;
        directEvent.wi = light_sample.d;
        Vector3f albedo = info.bsdf->weight(directEvent); 

        // fabs: sometimes bsdf is transparent, and the normal is flipped
        emission += throughput.cwiseProduct(albedo.cwiseProduct(light_sample.weight))
                  * std::fabs(light_sample.d.dot(event.normal));
      }
    }

    // bsdf sample 
    info.bsdf->sample(event); // 填充wi, pdf, rad

    // self-emission, assume that only outside emission is visible 
    if(info.primitive->getEmission()) {
      emission += throughput.cwiseProduct((*info.primitive->getEmission())[info.uv] * std::max(0.0f, event.normal.dot(event.wi)));
    }

    throughput = throughput.cwiseProduct((event.weight));
    float survival = throughput.maxCoeff();

    if (sampler.next1D() > survival) 
      return false; 
    throughput /= survival;

    return true; 
}

bool PathTracer::handleVolume(SurfaceScatterEvent& event, 
    MediumSample& sample, 
    Medium& medium, Vector3f& throughput, Vector3f& emission, 
    Ray& ray, 
    Scene& scene, 
    Sampler& sampler, Sampler& medium_sampler) {

    const IntersectionInfo& info = *event.info;

    // NEE (next event estimation) — same logic as handleSurface
    LightSample light_sample;
    if(scene.chooseLight(info.p, sampler, light_sample)) {
      Ray shadowRay(info.p + event.normal * g_pathTracerSettings.eps, light_sample.d);
      float bias_dist = light_sample.dist - g_pathTracerSettings.eps * std::abs(light_sample.d.dot(event.normal));
      shadowRay.setFarT(bias_dist * 0.99f);
      auto trans = medium.transmittance(shadowRay, sample, medium_sampler);

      SurfaceScatterEvent directEvent = event;
      directEvent.wi = light_sample.d;
      Vector3f f = info.bsdf->weight(directEvent);
      emission += trans.cwiseProduct(
        throughput.cwiseProduct(f.cwiseProduct(light_sample.weight))
      ) * std::fabs(light_sample.d.dot(event.normal));
    }

    // bsdf sample
    info.bsdf->sample(event); // fills wi, pdf, weight

    throughput = throughput.cwiseProduct((event.weight));
    float survival = throughput.maxCoeff();

    if (sampler.next1D() > survival) 
      return false; 
    throughput /= survival;

    return true; 
    

  }

Vector3f PathTracer::trace(Vector2i pixel, Scene& scene, uint32_t seed, int spp) {
  auto pix_seed = make_seed(pixel.x(), pixel.y(), spp, seed);
  // auto pix_seed = seed; 
  UniformPathSampler path_sampler(pix_seed);

  Ray ray = scene.cam().generateRay(pixel.x(), pixel.y());

  Vector3f emission = Vector3f::Zero();
  Vector3f throughput = Vector3f::Ones() ; 
  bool hasHit = false; 
  int bounce_times = 0; 

  GPConditioningState conditioning;
  MediumSample sample; // put it outside because it might has a context
  sample.conditioning = &conditioning; 

  do {
    hasHit = false; 
    bool exited = true;  
    // begin medium tracing
    if(getMedium()) {
      Medium* medium = getMedium().get();
      GPMedium* gpmedium = dynamic_cast<GPMedium*>(medium);

      uint32_t medium_seed = make_seed(pixel.x(), pixel.y(), spp, seed, bounce_times);
      ConstantSampler medium_sampler(g_gpSettings.gpMode == GPSettings::GPCorrelationMode::SingleRealization ? seed : medium_seed); // TODO: assume all the medium use constant sampler like GP Medium
      medium->sampleDistance(ray, sample, medium_sampler); // TODO: assume all the medium use constant sampler like GP Medium
      if(!sample.exited) {

        IntersectionInfo info; 
        SurfaceScatterEvent event = medium->makeSurfaceEventFromMedium(sample, info, ray, path_sampler, medium_sampler); // path sampler

        if(!handleVolume(event, sample, *medium, throughput, emission, ray, scene, path_sampler, medium_sampler)) {
          break; 
        }

        if(gpmedium) {
          ConstantSampler next_medium_sampler(g_gpSettings.gpMode == GPSettings::GPCorrelationMode::SingleRealization ? seed : make_seed(pixel.x(), pixel.y(), spp, seed, bounce_times + 1));
          gpmedium->sampleCondition(sample, next_medium_sampler);
        }

        // not sure is there should be backside
        ray = Ray(info.p + info.Ng * g_pathTracerSettings.eps, event.wi);
        ray.setNearT(g_pathTracerSettings.nearteps); 
        bounce_times ++; 
        exited = false; 
        hasHit = true;

      }
    }

    if(exited) {
      // begin hard surface tracing 
      IntersectionTemporary data; 
      IntersectionInfo info;
      scene.intersect(ray, data, info);

      if(data.primitive) {

        SurfaceScatterEvent event = makeSurfaceScatterEvent(info, ray, path_sampler);
        if(!handleSurface(event, throughput, emission, ray, scene, path_sampler)) {
          break; // rr 
        }

        bool wibackside = event.wi.dot(event.normal) < 0;
        float backside = wibackside ? -1.0f : 1.0f;
        if(wibackside) 
          setMedium(data.primitive->getIntMedium());
        else setMedium(data.primitive->getExtMedium());

        ray = Ray(info.p + backside * info.Ng * g_pathTracerSettings.eps, event.wi);
        ray.setNearT(g_pathTracerSettings.nearteps); // set because sphere's t approx to 0, lead to self-reflection
        bounce_times ++; 
        hasHit = true;
      } else { // skydrome
        if(scene.getSkydrome()) {
          IntersectionTemporary data; 
          IntersectionInfo info;
          scene.getSkydrome()->intersect(ray, data);
          scene.getSkydrome()->intersectInfo(data, info);
          Vector3f emis = (*scene.getSkydrome()->getEmission())[info.uv];
          emission += throughput.cwiseProduct(emis);
          // std::cout << "skydrome emission: " << emis.transpose() << std::endl;
        }
      } 
    }


  }while(hasHit && bounce_times < g_pathTracerSettings.max_bounce); 

  return emission;
}
}