#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <eigen3/Eigen/Eigen>
#include "../geometry/scene.hpp"
#include "camera.h"
using namespace Eigen;

namespace mupsi
{

  struct Color
  {
    Vector3f rgb;
  };

  class Renderer
  {
  public:
    Renderer(int width, int height);

    void render(const Scene &scene, const Camera &camera);

    void save(const std::string &filename) const;

  private:
    std::vector<Color> framebuffer;
    int width, height;

    int getidx(int x, int y) const;
  };

}

#endif