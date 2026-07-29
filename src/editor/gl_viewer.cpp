#include "gl_viewer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <cstdio>

namespace mupsi {

GlViewer::GlViewer(int fbW, int fbH, int winW, int winH, const std::string& title)
    : fbW_(fbW), fbH_(fbH), winW_(winW), winH_(winH)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(winW_, winH_, title.c_str(), nullptr, nullptr);
    if (!window_) return;

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glGenTextures(1, &texID_);
    glBindTexture(GL_TEXTURE_2D, texID_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    texData_.resize(fbW_ * fbH_ * 3, 0.0f);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbW_, fbH_, 0, GL_RGB, GL_FLOAT, texData_.data());
}

GlViewer::~GlViewer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (texID_) glDeleteTextures(1, &texID_);
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

bool GlViewer::shouldClose() const {
    return window_ && glfwWindowShouldClose(window_);
}

void GlViewer::beginFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GlViewer::uploadTexture(const float* tonemappedRGB, int w, int h) {
    int n = w * h * 3;
    if ((int)texData_.size() < n) texData_.resize(n);
    std::copy(tonemappedRGB, tonemappedRGB + n, texData_.begin());
    glBindTexture(GL_TEXTURE_2D, texID_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, texData_.data());
}

void GlViewer::endFrame() {
    ImGui::Render();
    int dsW, dsH;
    glfwGetFramebufferSize(window_, &dsW, &dsH);
    glViewport(0, 0, dsW, dsH);
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
    framerate_ = ImGui::GetIO().Framerate;
}

} // namespace mupsi
