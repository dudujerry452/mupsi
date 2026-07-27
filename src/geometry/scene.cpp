#include "scene.h"
#include <memory>
#include <iostream>
#include <utility>
#include "gp/gpnoise.h"
#include "rendering/trace.h"



#include "primitive.h"

using namespace mupsi;

namespace mupsi {


void Scene::addPrimitive(std::shared_ptr<Primitive> primitive) {
  primitives_.push_back(primitive);
}

void Scene::addBsdf(std::shared_ptr<Bsdf> bsdf) {
  bsdfs_.push_back(bsdf);
}

bool Scene::intersect(Ray& ray, IntersectionTemporary& data, IntersectionInfo& info) const {
  float mint = std::numeric_limits<float>::max();
  bool ret = false; 
  for(const auto& primitive: primitives_) {
    primitive->intersect(ray, data);
    if(data.primitive && ray.farT() < mint) {
      mint = ray.farT();
      info.p = ray.origin() + ray.direction() * ray.farT();
      info.t = ray.farT();
      primitive->intersectInfo(data, info);
      ret = true; 
    } 
  }
  return ret; 
}

bool Scene::occluded(const Ray& ray) const {
  for(const auto& primitive: primitives_) {
    if(primitive->occluded(ray)) {
      return true; 
    }
  }
  return false; 
}

bool Scene::chooseLight(const Vector3f& p, Sampler& sampler, LightSample& sample) const {
  if(primitives_.empty()) return false; 
  std::vector<std::shared_ptr<Primitive>> lightPrimitives;
  for(const auto& primitive: primitives_) {
    if(primitive->getEmission()) {
      lightPrimitives.push_back(primitive);
    }
  }
  if(lightPrimitives.empty()) return false;
  int index = sampler.nextI() % lightPrimitives.size(); 
  return lightPrimitives[index]->sampleDirect(p, sampler, sample);
}
}



