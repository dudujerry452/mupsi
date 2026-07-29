#pragma once

#include "framebuffer.h"
#include "camera.h"
#include "geometry/scene.h"
#include <atomic>
#include <functional>

namespace mupsi {

class Renderer {
    std::shared_ptr<Framebuffer> framebuffer_;

    std::atomic<int> done_{0};

    public:

    Renderer() = default;
    virtual ~Renderer() = default;

    void setCancelFlag(std::atomic<bool>* flag) { cancel_ = flag; }

    void prepareRender(Scene& scene);
    void startRender(Scene& scene, int spp);
    // Progressive: runs spp outer loop, calls onSpp(k+1) after each sample pass.
    // Returns true if completed, false if cancelled.
    bool startRenderProgressive(Scene& scene, int targetSpp,
                                std::function<void(int currentSpp)> onSpp = {});
    void afterRender();

    std::shared_ptr<Framebuffer> getFramebuffer() const { return framebuffer_; }

    int getProgress() const {
        if (!framebuffer_) return 0;
        int w = framebuffer_->width(), h = framebuffer_->height();
        int total = w * h;
        int done = done_.load();
        if (total == 0) return 0; return (100 * done / total);
    }

private:
    std::atomic<bool>* cancel_ = nullptr;
};

} // namespace mupsi
