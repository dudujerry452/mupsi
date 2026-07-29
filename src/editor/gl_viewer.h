#ifndef _EDITOR_GL_VIEWER_H_
#define _EDITOR_GL_VIEWER_H_

#include <string>
#include <vector>
#include <functional>

struct GLFWwindow;

namespace mupsi {

class Controller;

// GLFW + ImGui + OpenGL wrapper.
// Owns the window and framebuffer texture.
// Calls onFrame() each iteration so the editor can update state.
class GlViewer {
public:
    GlViewer(int fbW, int fbH, int winW, int winH, const std::string& title);
    ~GlViewer();

    GlViewer(const GlViewer&) = delete;
    GlViewer& operator=(const GlViewer&) = delete;

    GLFWwindow* window() const { return window_; }
    unsigned int texID() const { return texID_; }
    float       framerate() const { return framerate_; }
    bool        shouldClose() const;

    // Main loop iteration: poll events, new ImGui frame, upload texture, render.
    // Calls onGui() to populate ImGui panels, then renders the framebuffer texture.
    void beginFrame();
    void uploadTexture(const float* tonemappedRGB, int w, int h);
    void endFrame();

private:
    GLFWwindow* window_ = nullptr;
    unsigned int texID_ = 0;
    int  fbW_, fbH_, winW_, winH_;
    float framerate_ = 0.0f;
    std::vector<float> texData_;
};

} // namespace mupsi

#endif
