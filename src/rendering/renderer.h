#pragma once

#include "framebuffer.h"
#include "camera.h"
#include "../geometry/scene.h"

namespace mupsi {

class Renderer{
public: 
    Renderer(int width, int height); 
    void save(const std::string& filename) const { fb_.save(filename); }
protected: 
    Framebuffer fb_;
}; 

class SDFRenderer: public Renderer {
public:
    SDFRenderer(int width, int height);

    void render(const SDFScene& scene, const Camera& camera);

};

class GPRenderer: public Renderer {
public:
    GPRenderer(int width, int height);

    void render(const GPScene& scene, const Camera& camera);
};

} // namespace mupsi
