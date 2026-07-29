#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include <mutex>

namespace mupsi {

class Scene;
class Renderer;
class Primitive;
class Framebuffer;

class Controller {

std::shared_ptr<Scene> scene_;
std::shared_ptr<Renderer> renderer_;

std::shared_ptr<Framebuffer> framebufferFront_, framebufferBack_;
mutable std::mutex displayMtx_;

std::thread renderThread_;
std::atomic<bool> cancel_{false};
std::atomic<bool> shutdown_{false};

std::atomic<bool> sppPassDone_{false};
std::atomic<int>  currentSpp_{0};

int  spp_ = 1;
std::string outputPath_ = "test.png";

public:

Controller() = default;
~Controller() = default;

bool load(std::string config_path);

// Full render (all spp, blocks render thread until done, then swaps).
void start();

// Progressive — caller polls isSppPassDone() to know when a new frame arrived.
// Swaps framebufferFront/Back internally so getFrameBuffer() is always stable.
void startProgressive(int targetSpp, bool skipMedium = false);

void cancel();
void stop();

// True after each SPP pass — caller reads, restarts, etc.
// Controller never resets this; caller should:
//   bool done = ctrl.isSppPassDone(); Ctrl.ackSppPass();
bool isSppPassDone() const { return sppPassDone_.load(); }
void ackSppPass() { sppPassDone_.store(false); }

int  getCurrentSpp() const { return currentSpp_.load(); }

// Thread-safe: locks displayMtx_, copies tonemapped RGB to flat array [w*h*3].
void copyDisplayTo(float* dst, int w, int h) const;

// Raw (un-tonemapped) copy for saving results.
void copyDisplayRawTo(float* dst, int w, int h) const;

// Save current display framebuffer to file.
void saveDisplay(const std::string& path) const;

std::shared_ptr<Scene> getScene() const {return scene_; }
std::shared_ptr<Renderer> getRenderer() const {return renderer_; }

int  getSpp() const { return spp_; }
void setOutputPath(const std::string& path) { outputPath_ = path; }
const std::string& outputPath() const { return outputPath_; }

};

}

#endif
