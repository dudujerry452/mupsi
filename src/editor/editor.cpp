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
    int activeTab = -1; // -1 = live preview, >=0 = saved image index

    std::vector<float> texBuf; // for copyDisplayTo

    auto launchPreview = [&]() {
        isPreview   = true;
        needRestart = false;
        controller.ackSppPass();
        // Use preview camera resolution
        auto prevCam = std::make_shared<Camera>(
            scene.cam().pos(), scene.cam().dir(), Vector3f(0,1,0),
            scene.cam().fov(), previewW, previewH);
        scene.setCamera(prevCam);
        controller.startProgressive(previewSpp, true);
    };

    auto launchFull = [&]() {
        isPreview   = false;
        needRestart = false;
        controller.ackSppPass();
        auto fullCam = std::make_shared<Camera>(
            scene.cam().pos(), scene.cam().dir(), Vector3f(0,1,0),
            scene.cam().fov(), targetW, targetH);
        scene.setCamera(fullCam);
        controller.startProgressive(fullSpp, false);
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
        int cw = isPreview ? previewW : targetW;
        int ch = isPreview ? previewH : targetH;
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
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
            if (activeTab >= 0 && activeTab < (int)savedImages.size()) {
                auto& img = savedImages[activeTab];
                // Save as PNG
                std::string fname = img.title + ".png";
                for (auto& c : fname) if (c == ':' || c == ' ') c = '_';
                // Reconstruct Framebuffer from saved pixels and use its PNG saver
                Framebuffer fb(img.w, img.h);
                for (int y = 0; y < img.h; y++)
                    for (int x = 0; x < img.w; x++) {
                        int i = (y * img.w + x) * 3;
                        fb(x, y).rgb = Vec3f(img.pixels[i+0], img.pixels[i+1], img.pixels[i+2]);
                    }
                fb.save(fname);
                std::cout << "Saved " << fname << std::endl;
            }
        }

        // --- When full render completes, capture result to tab ---
        if (!isPreview && controller.isSppPassDone() &&
            controller.getCurrentSpp() >= fullSpp) {
            std::time_t t = std::time(nullptr);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

            SavedImage img;
            img.title = buf;
            img.spp   = fullSpp;
            img.w = targetW; img.h = targetH;
            img.pixels.resize(targetW * targetH * 3);
            controller.copyDisplayTo(img.pixels.data(), targetW, targetH);
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
                        targetW, targetH, prog);
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
            if (ImGui::BeginTabItem("Live")) {
                activeTab = -1;
                ImGui::EndTabItem();
            }
            for (int i = 0; i < (int)savedImages.size(); i++) {
                bool open = (activeTab == i);
                if (ImGui::BeginTabItem(savedImages[i].title.c_str(), &open)) {
                    activeTab = i;
                    ImGui::EndTabItem();
                }
                if (!open) {
                    savedImages.erase(savedImages.begin() + i);
                    if (activeTab >= (int)savedImages.size()) activeTab = (int)savedImages.size() - 1;
                    break;
                }
            }
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
