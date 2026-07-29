#include "editor.h"
#include "controller/controller.h"
#include "rendering/trace.h"
#include "geometry/scene.h"
#include "gl_viewer.h"
#include "camera_controller.h"

#include "rendering/camera.h"
#include "rendering/renderer.h"
#include "geometry/scene.h"
#include "rendering/framebuffer.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <cmath>
#include <chrono>
#include <ctime>
#include <iostream>

namespace mupsi {

// =========================================================================
struct SavedImage {
    std::string title;
    int         spp = 0;
    std::vector<float> pixels; // tonemapped RGB, flat [w*h*3]
    int w = 0, h = 0;
    bool open = true; // for ImGui tab close button
};

// =========================================================================
int runEditor(Controller& controller, const std::string& windowTitle) {
    Scene& scene = *controller.getScene();

    // --- Display constants ---
    constexpr int winW = 1280, winH  = 800;
    constexpr int previewW = 256, previewH = 256;
    constexpr int previewSpp = 4;
    constexpr float panelW = winW * 0.25f;  // right 1/4

    // --- GL init ---
    GlViewer viewer(previewW, previewH, winW, winH, windowTitle);

    // --- Camera ---
    CameraController camCtrl;
    camCtrl.syncFromCamera(scene.cam());

    int targetW = scene.cam().width();
    int targetH = scene.cam().height();
    int fullSpp = controller.getSpp();

    // --- State ---
    bool  isPreview    = true;
    bool  needRestart  = false;
    float lastFrameTime = float(glfwGetTime());

    std::vector<SavedImage> savedImages;
    int activeTab = -1;

    // Snapshot of full-render parameters (immutable while rendering)
    int  fullTargetSpp = fullSpp;
    int  fullTargetW   = targetW;
    int  fullTargetH   = targetH;

    std::vector<float> texBuf;

    auto launchPreview = [&]() {
        isPreview   = true;
        needRestart = false;
        controller.ackSppPass();
        auto prevCam = std::make_shared<Camera>(
            scene.cam().pos(), scene.cam().dir(), Vector3f(0,1,0),
            scene.cam().fov(), previewW, previewH);
        scene.setCamera(prevCam);
        controller.startProgressive(previewSpp, true);
    };

    auto launchFull = [&]() {
        isPreview     = false;
        needRestart   = false;
        fullTargetSpp = fullSpp;        // snapshot mutable params
        fullTargetW   = targetW;
        fullTargetH   = targetH;
        controller.ackSppPass();
        auto fullCam = std::make_shared<Camera>(
            scene.cam().pos(), scene.cam().dir(), Vector3f(0,1,0),
            scene.cam().fov(), fullTargetW, fullTargetH);
        scene.setCamera(fullCam);
        controller.startProgressive(fullTargetSpp, false);
    };

    launchPreview();

    // --- Main loop ---
    while (!viewer.shouldClose()) {
        viewer.beginFrame();

        float now = float(glfwGetTime());
        float dt  = now - lastFrameTime;
        lastFrameTime = now;
        dt = std::clamp(dt, 0.0f, 1.0f / 30.0f);

        // --- Camera ---
        camCtrl.apply(viewer.window(), dt);
        int cw = isPreview ? previewW : fullTargetW;
        int ch = isPreview ? previewH : fullTargetH;
        auto newCam = camCtrl.makeCamera(scene.cam(), dt, cw, ch);
        bool camMovedNow = ((newCam->pos() - scene.cam().pos()).squaredNorm() > 0.01f ||
                            (newCam->dir() - scene.cam().dir()).squaredNorm() > 0.0001f);
        if (camMovedNow && isPreview) {
            scene.setCamera(newCam);
            needRestart = true;
        }

        if (needRestart && controller.isSppPassDone()) {
            launchPreview();
        }

        // --- Space → full render ---
        static bool spaceWasDown = false;
        bool spaceDown = glfwGetKey(viewer.window(), GLFW_KEY_SPACE) == GLFW_PRESS;
        if (!spaceDown && spaceWasDown) {
            launchFull();
        }
        spaceWasDown = spaceDown;

        // --- Ctrl+S → save active tab ---
        bool ctrlDown = glfwGetKey(viewer.window(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
                     || glfwGetKey(viewer.window(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        bool sKeyDown = glfwGetKey(viewer.window(), GLFW_KEY_S) == GLFW_PRESS;
        static bool ctrlSWasDown = false;
        bool ctrlSNow = ctrlDown && sKeyDown;
        if (ctrlSNow && !ctrlSWasDown) {
            if (activeTab >= 0 && activeTab < (int)savedImages.size()) {
                auto& img = savedImages[activeTab];
                std::string fname = img.title + ".png";
                for (auto& c : fname) if (c == ':' || c == ' ') c = '_';
                Framebuffer fb(img.w, img.h);
                for (int y = 0; y < img.h; y++)
                    for (int x = 0; x < img.w; x++) {
                        int i = (y * img.w + x) * 3;
                        // SavedImage stores raw (un-tonemapped) values
                        fb(x, y).rgb = Vec3f(img.pixels[i+0], img.pixels[i+1], img.pixels[i+2]);
                    }
                fb.save(fname);
                printf("Saved %s\n", fname.c_str());
            }
        }
        ctrlSWasDown = ctrlSNow;

        // --- When full render completes, capture result to tab ---
        if (!isPreview && controller.isSppPassDone() &&
            controller.getCurrentSpp() >= fullTargetSpp) {
            printf("CAPTURE: currentSpp=%d fullTargetSpp=%d w=%d h=%d\n",
                controller.getCurrentSpp(), fullTargetSpp, fullTargetW, fullTargetH);
            std::time_t t = std::time(nullptr);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

            SavedImage img;
            img.title = buf;
            img.spp   = fullTargetSpp;
            img.w = fullTargetW; img.h = fullTargetH;
            img.pixels.resize(fullTargetW * fullTargetH * 3);
            controller.copyDisplayRawTo(img.pixels.data(), fullTargetW, fullTargetH);
            savedImages.push_back(std::move(img));
            activeTab = (int)savedImages.size() - 1;

            launchPreview();
        }

        // --- Upload texture from active source ---
        int texW = previewW, texH = previewH;
        if (activeTab >= 0 && activeTab < (int)savedImages.size()) {
            auto& img = savedImages[activeTab];
            texW = img.w; texH = img.h;
            viewer.uploadTexture(img.pixels.data(), texW, texH);
        } else {
            texBuf.resize(previewW * previewH * 3);
            if (isPreview) {
                controller.copyDisplayTo(texBuf.data(), previewW, previewH);
            }
            viewer.uploadTexture(texBuf.data(), previewW, previewH);
        }

        // --- Settings panel (right 1/4) ---
        float viewW_ = winW - panelW;
        float viewH_ = float(winH);

        ImGui::SetNextWindowPos(ImVec2(winW - panelW, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelW, float(winH)), ImGuiCond_Always);
        ImGui::Begin("Settings", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("FPS: %.1f", viewer.framerate());

        if (isPreview) {
            ImGui::Text("Preview | %dx%d | SPP %d/%d",
                        previewW, previewH, controller.getCurrentSpp(), previewSpp);
        } else {
            int prog = 0;
            if (controller.getRenderer())
                prog = controller.getRenderer()->getProgress();
            ImGui::Text("Full    | %dx%d | %d%%",
                        fullTargetW, fullTargetH, prog);
            ImGui::ProgressBar(float(prog) / 100.0f, ImVec2(-1, 0));
        }

        ImGui::Separator();

        int bounce = g_pathTracerSettings.max_bounce;
        if (ImGui::SliderInt("Max Bounce", &bounce, 1, 20)) {
            g_pathTracerSettings.max_bounce = bounce;
            if (isPreview) launchPreview();
        }

        int fs = fullSpp;
        if (ImGui::SliderInt("Full SPP", &fs, 1, 256))
            fullSpp = fs;

        if (ImGui::SliderInt("Target W", &targetW, 64, 2048)) {}
        if (ImGui::SliderInt("Target H", &targetH, 64, 2048)) {}

        ImGui::Text("Camera: %.0f %.0f %.0f",
            scene.cam().pos().x(), scene.cam().pos().y(), scene.cam().pos().z());
        ImGui::Text("RMB=look  WASD=move  SPACE=render");
        ImGui::Text("Ctrl+S = save tab");
        ImGui::End();

        // --- Viewport with tabs ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(viewW_, viewH_));
        ImGui::Begin("Viewport", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        if (ImGui::BeginTabBar("Tabs")) {
            if (ImGui::TabItemButton("Live")) {
                activeTab = -1;
            }
            for (int i = 0; i < (int)savedImages.size(); i++) {
                auto& si = savedImages[i];
                if (ImGui::BeginTabItem(si.title.c_str(), &si.open)) {
                    activeTab = i;
                    ImGui::EndTabItem();
                }
            }
            // Cleanup closed tabs
            savedImages.erase(
                std::remove_if(savedImages.begin(), savedImages.end(),
                    [](const SavedImage& s) { return !s.open; }),
                savedImages.end());
            if (activeTab >= (int)savedImages.size()) activeTab = (int)savedImages.size() - 1;
            if (savedImages.empty()) activeTab = -1;
            ImGui::EndTabBar();
        }

        // Scale and draw texture
        float scale = std::min(viewW_ / float(texW), (viewH_ - 30.0f) / float(texH));
        float imgW  = float(texW) * scale;
        float imgH  = float(texH) * scale;
        float padX  = (viewW_ - imgW) * 0.5f;
        float padY  = (viewH_ - 30.0f - imgH) * 0.5f + 30.0f;

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
