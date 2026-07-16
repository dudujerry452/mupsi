#pragma once

#include "framebuffer.h"
#include "camera.h"
#include "../geometry/scene.h"

namespace mupsi {

class SDFRenderer {
public:
    SDFRenderer(int width, int height);

    void render(const SDFScene& scene, const Camera& camera);
    void save(const std::string& filename) const { fb_.save(filename); }

private:
    Framebuffer fb_;
};

class GPRenderer {
public:
    GPRenderer(int width, int height);

    void render(const SDFScene& scene, const Camera& camera);
    void save(const std::string& filename) const { fb_.save(filename); }

private:
    Framebuffer fb_;
};

} // namespace mupsi
