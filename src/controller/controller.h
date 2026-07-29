#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include <functional>



namespace mupsi {

class Scene;
class Renderer;
class Primitive;
class Framebuffer;


class Controller {

std::shared_ptr<Scene> scene_;
std::shared_ptr<Renderer> renderer_;

std::shared_ptr<Framebuffer> framebufferFront_, framebufferBack_;
std::atomic<bool> frameReady_{false};

std::thread renderThread_;
std::atomic<bool> cancel_{false};
std::atomic<bool> shutdown_{false};

int  spp_ = 1;
std::string outputPath_ = "test.png";

public:

Controller() = default;
~Controller() = default;

bool load(std::string config_path);

// Full render (all spp at once, save on completion).
void start();
// Progressive render — fires onFrame callback each SPP pass with the Renderer's framebuffer.
// Caller is responsible for copying data out of the framebuffer inside onFrame
// (the framebuffer is still being written to between callbacks).
void startProgressive(int targetSpp,
                       std::function<void(const Framebuffer& fb, int currentSpp)> onFrame);
void cancel();
void stop();

bool isFrameReady() const;
const Framebuffer& getFrameBuffer() const;
void consumeFrameBuffer();

std::shared_ptr<Scene> getScene() const {return scene_; }
std::shared_ptr<Renderer> getRenderer() const {return renderer_; }

int getSpp() const { return spp_; }
void setOutputPath(const std::string& path) { outputPath_ = path; }
const std::string& outputPath() const { return outputPath_; }

};

}

#endif
