#include "renderer.h"
#include "trace.h"
#include <iostream>
#include <atomic>

namespace mupsi
{

  Renderer::Renderer(int width, int height): fb_(width, height) {}

  SDFRenderer::SDFRenderer(int width, int height): Renderer(width, height) {}

  void SDFRenderer::render(const SDFScene &scene, const Camera &camera)
  {
    int total = fb_.height() * fb_.width();
    std::atomic<int> done{0};

    #pragma omp parallel for collapse(2)
    for (int j = 0; j < fb_.height(); j++)
    {
      for (int i = 0; i < fb_.width(); i++)
      {

        float x = (i + 0.5f) / fb_.width();
        float y = (j + 0.5f) / fb_.height();

        Ray ray = camera.generateRay(x, y);

        // calculate intersection

        Intersection its = castRay(ray, scene);
        if (its.hit)
        {
          Vector3f light(-1, -1, -0.5);
          Vector3f lightemit(0.0, 1.0, 0.0);
          fb_(i, j) = Color({lightemit * std::max(0.0f, (light.normalized()).dot(its.normal))});
        }

        int n = ++done;
        if (n % (total / 100) == 0) {
          #pragma omp critical
          std::cerr << "\r" << (100 * n / total) << "%" << std::flush;
        }
      }
    }
    std::cerr << "\r100%" << std::endl;
  }


} // namespace mupsi
