#include "renderer.h"

namespace mupsi
{

  Renderer::Renderer(int width, int height)
      : fb_(width, height)
  {
  }

  void Renderer::render(const SDFScene &scene, const Camera &camera)
  {
    for (int j = 0; j < fb_.height(); j++)
    {
      for (int i = 0; i < fb_.width(); i++)
      {
        float x = (i + 0.5f) / fb_.width();
        float y = (j + 0.5f) / fb_.height();

        Ray ray = camera.generateRay(x, y);

        // TODO: ray marching + shading

        float t = 0.0, dt = 0.01;
        int depth = 50;

        bool flag = 0;
        for (int i = 0; i < depth; i++)
        {
          t += dt;
          Vector3f pos = ray.getOrigin() +
                         ray.getDirection() * t;
          float v = scene.eval(pos);
        }
      }
    }
  }

} // namespace mupsi
