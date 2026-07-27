#pragma once

#include "framebuffer.h"
#include "camera.h"
#include "geometry/scene.h"

namespace mupsi {

class Renderer {
    std::shared_ptr<Framebuffer> framebuffer_;

    public: 

    Renderer() = default; 
    virtual ~Renderer() = default;

    void prepareRender(Scene& scene);
    void startRender(Scene& scene, int spp); 
    void afterRender(); 

}; 

} // namespace mupsi
