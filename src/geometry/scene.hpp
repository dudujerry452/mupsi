#ifndef _SCENE_HPP_
#define _SCENE_HPP_

#include "object.hpp"
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

  class Scene
  {
  public:
    Scene();
    virtual ~Scene() = default;

    void add(std::unique_ptr<Object> obj);

  protected:
    std::vector<std::unique_ptr<Object>> objects;
  };

  class SDFScene : public Scene
  {
  public:
    SDFScene();
    virtual ~SDFScene() = default;

    float eval(const Vector3f &pos) const;
    Intersection castRay(const Ray &ray) const;
  };
}

#endif