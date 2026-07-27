#ifndef _SCENE_H_
#define _SCENE_H_

#include "ray.h"
#include "gp/gpnoise.h"
#include "bsdf/bsdf.h"
#include <memory>
#include <vector>

namespace mupsi
{

  // new: 

  class Primitive;
  class Bsdf; 
  class Camera; 
  class Medium;

  class IntersectionTemporary;
  class IntersectionInfo;

  class Scene
  {

      std::vector<std::shared_ptr<Primitive>> primitives_;
      std::vector<std::shared_ptr<Bsdf>> bsdfs_; // owner 

      std::shared_ptr<Camera> camera_;

      std::shared_ptr<Medium> medium_;

      public: 

      Scene() = default; 
      virtual ~Scene() = default;

      void addPrimitive(std::shared_ptr<Primitive> primitive);
      void addBsdf(std::shared_ptr<Bsdf> bsdf);
      void setCamera(std::shared_ptr<Camera> camera) { camera_ = camera; }

      bool intersect(Ray& ray, IntersectionTemporary& data, IntersectionInfo& info) const;
      bool occluded(const Ray& ray) const;
      bool chooseLight(const Vector3f& p, Sampler& sampler, LightSample& sample) const;

      void setMedium(std::shared_ptr<Medium> medium) { medium_ = medium; }
      std::shared_ptr<Medium> getMedium() const { return medium_; }

      const Camera& cam() const { return *camera_; }

      friend class Renderer; 

  }; 
}

#endif