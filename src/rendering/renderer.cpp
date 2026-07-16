#include "renderer.h"
#include <iostream>

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

        // calculate intersection

        Intersection its = scene.castRay(ray);
        if (its.hit)
        {
          Vector3f light(1, -1, 0); 
          Vector3f lightemit(0.0, 50.0, 0.0); 
          fb_(i, j) = Color({lightemit * light.dot(its.normal)}); 
        }
      }
    }
  }

} // namespace mupsi
