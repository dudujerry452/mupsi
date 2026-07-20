#ifndef _SCENE_H_
#define _SCENE_H_

#include "object.h"
#include "ray.h"
#include "gp/gpnoise.h"
#include <memory>
#include <vector>

namespace mupsi
{

  struct Intersection
  {
    bool hit;
    float t;
    Vector3f position;
    Vector3f normal;
    std::shared_ptr<Material> material;
  };

  struct ParallelLight {
    Vector3f direction; 
    Vector3f intensity; 
  };

  class SDFScene
  {
  public:
    SDFScene();
    virtual ~SDFScene() = default;

    void add(std::unique_ptr<SDFObject> obj);
    void addParallelLight(const ParallelLight& light) { parallel_lights.push_back(light); };

    float eval(const Vector3f &pos) const { const SDFObject* obj = nullptr; return eval(pos, obj); };
    virtual float eval(const Vector3f &pos, const SDFObject*& obj) const;
    Vector3f getNormal(const Vector3f &pos) const;

    protected:
    std::vector<std::unique_ptr<SDFObject>> objects;
    Bounding bounding;

    std::vector<ParallelLight> parallel_lights;

    public: 
    friend Vector3f traceRay(const Ray &ray, const SDFScene &scene, int depth);
  };

  class GPScene: public SDFScene
  {
  public:
    GPScene(const GPNoiseGenerator& gp): SDFScene(), gpnoise(gp) {};
    virtual ~GPScene() = default;

    float eval(const Vector3f &pos, const SDFObject*& obj) const override;

  private:
    GPNoiseGenerator gpnoise;

  };
}

#endif