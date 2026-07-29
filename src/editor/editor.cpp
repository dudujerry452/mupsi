#include "editor.h"
#include "controller/controller.h"
#include "rendering/trace.h"
#include "geometry/scene.h"
#include "gl_viewer.h"
#include "camera_controller.h"

#include "rendering/camera.h"
#include "geometry/scene.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <cmath>

namespace mupsi {

static void handleUI(float fps, int currentSpp, bool previewActive,
                     int& previewSpp, int& fullSpp,
                     const Scene& scene,
                     std::function<void(int)> onRestart) {
    ImGui::SetNextWindowPos(ImVec2(float(1280) - 240.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(240.0f, float(800)), ImGuiCond_Always);
    ImGui::Begin("Settings", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("SPP: %d / %s", currentSpp,
        previewActive ? "preview" : "full");

    if (ImGui::SliderInt("Preview SPP", &previewSpp, 1, 16)) {
        if (previewActive) onRestart(previewSpp);
    }

    int bounce = g_pathTracerSettings.max_bounce;
    if (ImGui::SliderInt("Max Bounce", &bounce, 1, 20)) {
        g_pathTracerSettings.max_bounce = bounce;
        if (previewActive) onRestart(previewSpp);
    }

    int fs = fullSpp;
    if (ImGui::SliderInt("Full SPP", &fs, 1, 256))
        fullSpp = fs;

    ImGui::Text("Camera: %.0f %.0f %.0f",
        scene.cam().pos().x(), scene.cam().pos().y(), scene.cam().pos().z());
    ImGui::Text("RMB=look  WASD=move  SPACE=render");
    ImGui::End();
}

int runEditor(Controller& controller, const std::string& windowTitle) {
    Scene& scene = *controller.getScene();

    int fbW = scene.cam().width();
    int fbH = scene.cam().height();
    GlViewer viewer(fbW, fbH, 1280, 800, windowTitle);

    // --- State ---
    CameraController camCtrl;
    camCtrl.syncFromCamera(scene.cam());

    bool  previewActive = true;
    int   previewSpp    = 4;
    int   fullSpp       = controller.getSpp();
    float lastFrameTime = float(glfwGetTime());
    bool  needRestart   = false;

    std::vector<float> texBuf(fbW * fbH * 3, 0.0f); // for copyDisplayTo

    auto launchRender = [&](int targetSpp, bool isPreview) {
        previewActive = isPreview;
        needRestart   = false;
        controller.ackSppPass();
        controller.startProgressive(targetSpp);
    };

    launchRender(previewSpp, true);

    // --- Main loop ---
    while (!viewer.shouldClose()) {
        viewer.beginFrame();

        float now = float(glfwGetTime());
        float dt  = now - lastFrameTime;
        lastFrameTime = now;
        dt = std::clamp(dt, 0.0f, 1.0f / 30.0f);

        // --- Camera ---
        camCtrl.apply(viewer.window(), dt);
        auto newCam = camCtrl.makeCamera(scene.cam(), dt);
        bool camMovedNow = ((newCam->pos() - scene.cam().pos()).squaredNorm() > 0.01f ||
                            (newCam->dir() - scene.cam().dir()).squaredNorm() > 0.0001f);
        if (camMovedNow) {
            scene.setCamera(newCam);
            needRestart = true;
        }

        if (needRestart && controller.isSppPassDone()) {
            launchRender(previewSpp, true);
        }

        // Space → full render
        static bool spaceWasDown = false;
        bool spaceDown = glfwGetKey(viewer.window(), GLFW_KEY_SPACE) == GLFW_PRESS;
        if (!spaceDown && spaceWasDown && previewActive) {
            launchRender(fullSpp, false);
        }
        spaceWasDown = spaceDown;

        // --- Upload framebuffer ---
        controller.copyDisplayTo(texBuf.data(), fbW, fbH);
        viewer.uploadTexture(texBuf.data(), fbW, fbH);

        // --- Settings panel ---
        handleUI(viewer.framerate(), controller.getCurrentSpp(),
                 previewActive, previewSpp, fullSpp,
                 scene,
                 [&](int newPreviewSpp) { launchRender(newPreviewSpp, true); });
        const float panelW = 240.0f;

        // --- Viewport (auto-scaled) ---
        float viewW = 1280.0f - panelW;
        float viewH = 800.0f;
        float scale = std::min(viewW / float(fbW), viewH / float(fbH));
        float imgW  = float(fbW) * scale;
        float imgH  = float(fbH) * scale;
        float padX  = (viewW - imgW) * 0.5f;
        float padY  = (viewH - imgH) * 0.5f;

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(viewW, viewH));
        ImGui::Begin("Viewport", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetCursorPos(ImVec2(padX, padY));
        ImGui::Image((ImTextureID)(intptr_t)viewer.texID(), ImVec2(imgW, imgH),
                     ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();

        viewer.endFrame();
    }

    controller.stop();
    return 0;
}

} // namespace mupsi
