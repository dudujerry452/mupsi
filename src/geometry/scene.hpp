#ifndef _SCENE_HPP_
#define _SCENE_HPP_

#include "object.hpp"
#include <memory>
#include <vector>

namespace mupsi
{
  class Scene
  {
  public:
    Scene();

    void add(std::unique_ptr<SDFObject> obj);
    float eval(const Vector3f &pos) const;

  private:
    std::vector<std::unique_ptr<SDFObject>> objects;
  };
}

#endif