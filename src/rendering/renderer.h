#pragma once

#include "framebuffer.h"
#include "camera.h"
#include "geometry/scene.h"

namespace mupsi {

class Renderer0{
public: 
    Renderer0(int width, int height); 
    void save(const std::string& filename) const { fb_.save(filename); }
protected: 
    Framebuffer fb_;
}; 


// note: SDFRenderer can render both SDFScene and GPScene, GPScene's eval is overriden.
class SDFRenderer: public Renderer0 { 
public:
    SDFRenderer(int width, int height);

    void render(SDFScene& scene, const Camera& camera);

};

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
