#include "renderer.h"
#include "trace.h"
#include "geometry/primitive.h"
#include <iostream>
#include <atomic>

namespace mupsi
{

  void Renderer::prepareRender(const RenderContext& ctx) {
    framebuffer_ = std::make_shared<Framebuffer>(ctx.camera.width(), ctx.camera.height());

    for(auto& primitive: ctx.scene->primitives_) {
      primitive->prepareForRender();
    }
  }

  void Renderer::startRender(const RenderContext& ctx, int spp) {
    startRenderProgressive(ctx, spp);
  }

  bool Renderer::startRenderProgressive(const RenderContext& ctx, int targetSpp,
                                        std::function<void(int)> onSpp) {
    int w = ctx.camera.width(), h = ctx.camera.height();
    done_ = 0;
    targetSpp_ = targetSpp;

    for (int k = 0; k < targetSpp; k++) {
      if (cancel_ && cancel_->load()) return false;

      #pragma omp parallel for collapse(2)
      for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
          if (cancel_ && cancel_->load()) continue;

          PathTracer tracer;
          Vector3f e = tracer.trace(Vector2i(i, j), ctx, 42, k);
          framebuffer_->accumulate(i, j, e);
          ++done_;
        }
      }

      framebuffer_->incrementSampleCount();

      if (onSpp) onSpp(k + 1);
    }

    return true;
  }

  void Renderer::afterRender() {
    framebuffer_->save("test.png");
  }

} // namespace mupsi
