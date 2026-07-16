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
        // i = 127; j = 127; 

        float x = (i + 0.5f) / fb_.width();
        float y = (j + 0.5f) / fb_.height();

        Ray ray = camera.generateRay(x, y);
        // std::cout << "ray = " << ray.getDirection() << " " << std:: endl; 

        // calculate intersection

        Intersection its = scene.castRay(ray);
        if (its.hit)
        {
          Vector3f light(1, 1, 0.5); 
          Vector3f lightemit(0.0, 1.0, 0.0); 
          fb_(i, j) = Color({lightemit * std::max(0.0f, (-light.normalized()).dot(its.normal))}); 
        }
        // break; 
      }
      // break; 
    }
  }

} // namespace mupsi
