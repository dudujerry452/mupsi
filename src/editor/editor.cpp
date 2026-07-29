#include "editor.h"
#include "controller/controller.h"
#include "rendering/framebuffer.h"
#include "rendering/renderer.h"
#include "rendering/trace.h"
#include "rendering/camera.h"
#include "geometry/scene.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <thread>
#include <chrono>
#include <mutex>
#include <cstdio>
#include <cmath>

namespace mupsi {

// =========================================================================
// Camera controller: WASD + mouse look
// =========================================================================
struct CameraController {
    float yaw   = 0.0f;
    float pitch = 0.0f;
    float moveSpeed = 50.0f;
    float lookSpeed = 0.002f;

    // Default direction: looking at -Z
    void apply(GLFWwindow* window, float dt) {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        static int lastState = GLFW_RELEASE;
        int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
        if (state == GLFW_PRESS) {
            static double lastX = 0.0, lastY = 0.0;
            if (lastState == GLFW_RELEASE) { lastX = mx; lastY = my; }
            float dx = float(mx - lastX);
            float dy = float(my - lastY);
            yaw   += dx * lookSpeed;
            pitch -= dy * lookSpeed;
            pitch  = std::clamp(pitch, -1.4f, 1.4f);
            lastX = mx; lastY = my;
        }
        lastState = state;

        move_[0] = move_[1] = move_[2] = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move_.z() += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move_.z() -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move_.x() -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move_.x() += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) move_.y() += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) move_.y() -= 1.0f;
    }

    // Compute new camera from current yaw/pitch/move
    std::shared_ptr<Camera> makeCamera(const Camera& current, float dt) const {
        Vector3f dir(std::cos(pitch) * std::sin(yaw),
                     std::sin(pitch),
                    -std::cos(pitch) * std::cos(yaw));
        dir.normalize();
        Vector3f right = dir.cross(Vector3f(0.0f, 1.0f, 0.0f)).normalized();
        Vector3f up    = right.cross(dir).normalized();

        Vector3f dpos = (dir * move_[2] + right * move_[0] + up * move_[1]) * moveSpeed * dt;

        return std::make_shared<Camera>(
            current.pos() + dpos, dir, Vector3f(0.0f, 1.0f, 0.0f),
            current.fov(), current.width(), current.height()
        );
    }

    // Initialize yaw/pitch from current camera direction
    void syncFromCamera(const Camera& cam) {
        const Vector3f& d = cam.dir();
        yaw   = std::atan2(d.x(), -d.z());
        pitch = std::asin(std::clamp(d.y(), -1.0f, 1.0f));
    }

private:
    Vector3f move_{0.0f, 0.0f, 0.0f};
};

// =========================================================================
// Shared framebuffer (render thread <-> UI thread)
// =========================================================================
struct SharedFB {
    std::shared_ptr<Framebuffer> fb;
    std::mutex mtx;
    int  currentSpp = 0;
    bool updated    = false;

    void set(std::shared_ptr<Framebuffer> f, int spp) {
        std::lock_guard<std::mutex> lock(mtx);
        fb = f;
        currentSpp = spp;
        updated = true;
    }

    std::shared_ptr<Framebuffer> get() {
        std::lock_guard<std::mutex> lock(mtx);
        updated = false;
        return fb;
    }
};

// =========================================================================
// Editor main loop
// =========================================================================
int runEditor(Controller& controller, const std::string& windowTitle) {
    Scene& scene = *controller.getScene();

    // --- GLFW init ---
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int fbW = scene.cam().width();
    int fbH = scene.cam().height();
    GLFWwindow* window = glfwCreateWindow(fbW * 2, fbH + 60,
        windowTitle.c_str(), nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    // --- ImGui init ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // --- OpenGL texture ---
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    std::vector<float> texData(fbW * fbH * 3, 0.0f);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbW, fbH, 0, GL_RGB, GL_FLOAT, texData.data());

    // --- State ---
    CameraController camCtrl;
    camCtrl.syncFromCamera(scene.cam());

    std::atomic<bool> renderCancel{false};
    std::thread        renderThread;
    SharedFB           sharedFB;

    bool  previewActive = false;
    int   previewSpp    = 4;
    int   fullSpp       = controller.getSpp();
    float lastFrameTime = float(glfwGetTime());

    auto launchRender = [&](int targetSpp, bool isPreview) {
        if (renderThread.joinable()) {
            renderCancel.store(true);
            renderThread.join();
        }
        renderCancel.store(false);
        previewActive = isPreview;

        auto fb = std::make_shared<Framebuffer>(fbW, fbH);
        sharedFB.set(fb, 0);

        renderThread = std::thread([&, fb, targetSpp]() mutable {
            Renderer renderer;
            renderer.setCancelFlag(&renderCancel);
            renderer.prepareRender(scene);
            bool done = renderer.startRenderProgressive(scene, targetSpp,
                [&](int s) {
                    sharedFB.set(fb, s);
                });
            (void)done;
        });
    };

    launchRender(previewSpp, true);

    // --- Main loop ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        float now = float(glfwGetTime());
        float dt  = now - lastFrameTime;
        lastFrameTime = now;
        if (dt <= 0.0f) dt = 1.0f / 60.0f;

        // --- Camera update ---
        camCtrl.apply(window, dt);
        auto newCam = camCtrl.makeCamera(scene.cam(), dt);
        bool camChanged = ((newCam->pos() - scene.cam().pos()).squaredNorm() > 1.0f ||
                           (newCam->dir() - scene.cam().dir()).squaredNorm() > 0.0001f);

        if (camChanged) {
            scene.setCamera(newCam);
            launchRender(previewSpp, true);
        }

        // Space → full render
        static bool spaceWasDown = false;
        bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (!spaceDown && spaceWasDown && previewActive) {
            launchRender(fullSpp, false);
        }
        spaceWasDown = spaceDown;

        // --- Upload to GL texture ---
        auto fb = sharedFB.get();
        if (fb) {
            for (int y = 0; y < fbH; y++)
                for (int x = 0; x < fbW; x++) {
                    Vec3f c = fb->tonemapped(x, y);
                    int i = (y * fbW + x) * 3;
                    texData[i + 0] = c.x();
                    texData[i + 1] = c.y();
                    texData[i + 2] = c.z();
                }
            glBindTexture(GL_TEXTURE_2D, texID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbW, fbH, 0,
                         GL_RGB, GL_FLOAT, texData.data());
        }

        // --- ImGui panel ---
        ImGui::SetNextWindowPos(ImVec2(float(fbW) + 20.0f, 10.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(240.0f, float(fbH)), ImGuiCond_Once);
        ImGui::Begin("Settings");
        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("SPP: %d / %s", sharedFB.currentSpp,
            previewActive ? "preview" : "full");

        if (ImGui::SliderInt("Preview SPP", &previewSpp, 1, 16)) {
            if (previewActive) launchRender(previewSpp, true);
        }

        int bounce = g_pathTracerSettings.max_bounce;
        if (ImGui::SliderInt("Max Bounce", &bounce, 1, 20)) {
            g_pathTracerSettings.max_bounce = bounce;
            if (previewActive) launchRender(previewSpp, true);
        }

        int fs = fullSpp;
        if (ImGui::SliderInt("Full SPP", &fs, 1, 256)) {
            fullSpp = fs;
        }

        ImGui::Text("Camera: %.0f %.0f %.0f",
            scene.cam().pos().x(), scene.cam().pos().y(), scene.cam().pos().z());
        ImGui::Text("RMB=look  WASD=move  SPACE=render");
        ImGui::End();

        // --- Draw texture ---
        // Use ImGui to draw the framebuffer texture
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(float(fbW), float(fbH) + 40.0f));
        ImGui::Begin("Viewport", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        ImGui::Image((ImTextureID)(intptr_t)texID, ImVec2(float(fbW), float(fbH)),
                     ImVec2(0, 1), ImVec2(1, 0)); // flip Y
        ImGui::End();

        // --- Render ---
        ImGui::Render();
        int dsW, dsH;
        glfwGetFramebufferSize(window, &dsW, &dsH);
        glViewport(0, 0, dsW, dsH);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // --- Cleanup ---
    renderCancel.store(true);
    if (renderThread.joinable()) renderThread.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteTextures(1, &texID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

} // namespace mupsi
