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
        if (!framebuffer_ || targetSpp_ == 0) return 0;
        int w = framebuffer_->width(), h = framebuffer_->height();
        return (100 * done_.load()) / (w * h * targetSpp_);
    }

private:
    std::atomic<bool>* cancel_ = nullptr;
    int targetSpp_ = 0;
};

} // namespace mupsi
