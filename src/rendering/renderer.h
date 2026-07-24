#pragma once

#include "framebuffer.h"
#include "camera.h"
#include "geometry/scene.h"

namespace mupsi {

class Renderer{
public: 
    Renderer(int width, int height); 
    void save(const std::string& filename) const { fb_.save(filename); }
protected: 
    Framebuffer fb_;
}; 


// note: SDFRenderer can render both SDFScene and GPScene, GPScene's eval is overriden.
class SDFRenderer: public Renderer { 
public:
    SDFRenderer(int width, int height);

    void render(SDFScene& scene, const Camera& camera);

};


} // namespace mupsi
