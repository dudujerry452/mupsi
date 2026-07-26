#include "renderer.h"
#include "trace.h"
#include "geometry/primitive.h"
#include <iostream>
#include <atomic>

namespace mupsi
{

  Renderer0::Renderer0(int width, int height): fb_(width, height) {}

  SDFRenderer::SDFRenderer(int width, int height): Renderer0(width, height) {}

  void SDFRenderer::render(SDFScene &scene, const Camera &camera)
  {
    int total = fb_.height() * fb_.width();
    std::atomic<int> done{0};

    #pragma omp parallel for collapse(2)
    for (int j = 0; j < fb_.height(); j++)
    {
      for (int i = 0; i < fb_.width(); i++)
      {

        Ray ray = camera.generateRay(i, j);

        // set thread-local pixel context for per-bounce GP seed generation
        g_pixel_x = i;
        g_pixel_y = j;
        g_spp = 0;

        // calculate intersection

        Vector3f its = traceRay(ray, scene, 0);
        fb_(i, j) = Color({its});

        int n = ++done;
        if (n % (total / 100) == 0) {
          #pragma omp critical
          std::cerr << "\r" << (100 * n / total) << "%" << std::flush;
        }
      }
    }
    std::cerr << "\r100%" << std::endl;
  }

  void Renderer::prepareRender(Scene& scene) { 
    framebuffer_ = std::make_shared<Framebuffer>(scene.cam().width(), scene.cam().height());

    for(auto& primitive: scene.primitives_) {
      primitive->prepareForRender(); 
    }
  }

  void Renderer::startRender(Scene& scene, int spp) {
    int w = scene.cam().width(), h = scene.cam().height();
    int total = w * h;
    std::atomic<int> done{0};
    float inv_k = 1.0f / spp;
    #pragma omp parallel for collapse(2)
    for(int j = 0; j < h; j ++) {
      for(int i = 0; i < w; i ++) {
        Vector3f emmision = Vector3f::Zero();
        for(int k = 0; k < spp; k ++) {
          PathTracer tracer; 
          emmision += tracer.trace(Vector2i(i, j), scene, 0, k);
        }
        emmision *= inv_k;
        framebuffer_->operator()(i, j) = Color({emmision});

        int n = ++done;
        if (n % (total / 100) == 0) {
          #pragma omp critical
          std::cerr << "\r" << (100 * n / total) << "%" << std::flush;
        }
      }
    }
    std::cerr << "\r100%" << std::endl;
  }

  void Renderer::afterRender() { 
    framebuffer_->save("test.png");
  }

} // namespace mupsi
