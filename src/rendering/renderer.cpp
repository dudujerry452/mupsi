#include "renderer.h"
#include "trace.h"
#include "geometry/primitive.h"
#include <iostream>
#include <atomic>

namespace mupsi
{

  void Renderer::prepareRender(Scene& scene) {
    framebuffer_ = std::make_shared<Framebuffer>(scene.cam().width(), scene.cam().height());

    for(auto& primitive: scene.primitives_) {
      primitive->prepareForRender();
    }
  }

  void Renderer::startRender(Scene& scene, int spp) {
    startRenderProgressive(scene, spp);
  }

  bool Renderer::startRenderProgressive(Scene& scene, int targetSpp,
                                        std::function<void(int)> onSpp) {
    int w = scene.cam().width(), h = scene.cam().height();
    done_ = 0;

    for (int k = 0; k < targetSpp; k++) {
      if (cancel_ && cancel_->load()) return false;

      #pragma omp parallel for collapse(2)
      for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
          if (cancel_ && cancel_->load()) continue;

          PathTracer tracer;
          Vector3f e = tracer.trace(Vector2i(i, j), scene, 42, k);
          framebuffer_->accumulate(i, j, e);
        }
      }

      framebuffer_->incrementSampleCount();
      done_ = (k + 1) * w * h;

      if (onSpp) onSpp(k + 1);
    }

    return true;
  }

  void Renderer::afterRender() {
    framebuffer_->save("test.png");
  }

} // namespace mupsi
