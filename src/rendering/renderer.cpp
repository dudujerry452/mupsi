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
    int w = scene.cam().width(), h = scene.cam().height();
    float inv_k = 1.0f / spp;
    done_ = 0;
    #pragma omp parallel for collapse(2)
    for(int j = 0; j < h; j ++) {
      for(int i = 0; i < w; i ++) {
        if(cancel_ && cancel_->load()) {
          framebuffer_->operator()(i, j) = Color({Vector3f::Zero()});
          ++done_;
          continue;
        }
        Vector3f emmision = Vector3f::Zero();
        for(int k = 0; k < spp; k ++) {
          PathTracer tracer;
          emmision += tracer.trace(Vector2i(i, j), scene, 42, k);
        }
        emmision *= inv_k;
        framebuffer_->operator()(i, j) = Color({emmision});
        ++done_;
      }
    }
  }

  void Renderer::afterRender() { 
    framebuffer_->save("test.png");
  }

} // namespace mupsi
