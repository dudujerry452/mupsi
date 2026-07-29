#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <memory>
#include <atomic>
#include <thread>
#include <string>



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
void start(); // start render thread
void cancel(); // signal stop
void stop(); // join render thread

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
