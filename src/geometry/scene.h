#ifndef _SCENE_H_
#define _SCENE_H_

#include "object.h"
#include "ray.h"
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
  };

  class SDFScene
  {
  public:
    SDFScene();
    virtual ~SDFScene() = default;

    void add(std::unique_ptr<SDFObject> obj);

    float eval(const Vector3f &pos) const;
    virtual Intersection castRay(const Ray &ray) const;

    protected:
    std::vector<std::unique_ptr<SDFObject>> objects;
  };

  class GPScene: SDFScene
  {
  public:
    GPScene();
    virtual ~GPScene() = default;

    Intersection castRay(const Ray &ray) const override;
  };
}

#endif