#ifndef _SCENE_HPP_
#define _SCENE_HPP_

#include "object.hpp"

namespace mupsi
{
  class Scene
  {
  public:
    Scene();

    void add(std::unique_ptr<Object> obj);

  private:
    std::vector<std::unique_ptr<Object>> objects;
  };
}

#endif