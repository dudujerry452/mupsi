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
    virtual ~Renderer() = default;

    virtual void render(const Scene &scene, const Camera &camera) = 0;

    virtual void save(const std::string &filename) const;

  protected:
    std::vector<Color> framebuffer;
    int width, height;

    int getidx(int x, int y) const;
  };

  class SDFRenderer : public Renderer
  {
  public:
    SDFRenderer(int width, int height);
    virtual ~SDFRenderer() = default;

    void render(const Scene &scene, const Camera &camera) override;
  };

}

#endif