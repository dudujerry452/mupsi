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


std::shared_ptr<PathTracerSettings> PathTracer::settings_ = std::make_shared<PathTracerSettings>();

SurfaceScatterEvent PathTracer::makeSurfaceScatterEvent(IntersectionTemporary& data, IntersectionInfo& info, Ray& ray, UniformPathSampler& sampler) {
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

    bool backside = event.wo.dot(event.normal) < 0;
      if(backside) {
        event.normal = -event.normal;
      } 

    // 自发光
    const IntersectionInfo& info = *event.info;

    // notice: medium's primitive is nullptr, but handleSurface is only called when hit hard surface
    if(info.primitive->getEmission()) {
      emission += throughput.cwiseProduct((*info.primitive->getEmission())[info.uv]);
    }

    // light sample 
    // emission += throughput.cwiseProduct(Vector3f(1.0f, 1.0f, 0.0f)) * std::max(0.0f, Vector3f(1.0f, 0.0f, 0.0f).dot(event.normal)); 

    LightSample light_sample;
    if(scene.chooseLight(info.p, sampler, light_sample)) {
      Ray ray(info.p + event.normal * settings_->eps, light_sample.d);
      // important: fix farT because the origin point of ray is p + eps
      float bias_dist = light_sample.dist - settings_->eps * std::abs(light_sample.d.dot(event.normal));
      ray.setFarT(bias_dist * 0.99f);  // stop before light surface
      if(!scene.occluded(ray)) {
        emission += throughput.cwiseProduct(light_sample.weight) * std::max(0.0f, light_sample.d.dot(event.normal));
      }
    }

    // bsdf sample 
    info.bsdf->sample(event); // 填充wi, pdf, rad

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

    LightSample light_sample;
    if(scene.chooseLight(info.p, sampler, light_sample)) {
      Ray ray(info.p + event.normal * settings_->eps, light_sample.d);
      auto trans = medium.transmittance(ray, sample, medium_sampler);
      emission += trans.cwiseProduct(
        throughput.cwiseProduct(light_sample.weight) * std::max(0.0f, light_sample.d.dot(event.normal))
      ); 
    }

    // bsdf sample 
    info.bsdf->sample(event); // 填充wi, pdf, rad

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

  Medium* medium = scene.getMedium().get();
  GPMedium* gpmedium = dynamic_cast<GPMedium*>(medium);
  
  do {
    hasHit = false; 
    bool exited = true;  
    // begin medium tracing
    if(medium) {
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
          gpmedium->sampleCondition(sample, medium_sampler);
        }

        ray = Ray(info.p + info.Ng * settings_->eps, event.wi);
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


        SurfaceScatterEvent event = makeSurfaceScatterEvent(data, info, ray, path_sampler);
        if(!handleSurface(event, throughput, emission, ray, scene, path_sampler)) {
          break; 
        }

        ray = Ray(info.p + info.Ng * settings_->eps, event.wi);
        bounce_times ++; 
        hasHit = true;
      } 
    }


  }while(hasHit && bounce_times < settings_->max_bounce); 

  return emission;
}
}