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
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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

    // --- Preview settings (editable) ---
    int previewW = 256, previewH = 256;
    int previewSpp = 4;

    // --- GL init ---
    GlViewer viewer(previewW, previewH, 1280, 800, windowTitle);

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

    // Render FPS (SPP passes / sec)
    int   sppPassCount = 0;
    float sppTimer     = 0.0f;
    float renderFps    = 0.0f;

    // Save feedback toast
    float       saveToastTimer = 0.0f;
    std::string saveToastPath;

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
        fullTargetSpp = fullSpp;
        fullTargetW   = targetW;
        fullTargetH   = targetH;
        printf("=== Full Render ===\n");
        printf("  Camera: pos=(%.1f %.1f %.1f) dir=(%.3f %.3f %.3f) fov=%.1f\n",
               scene.cam().pos().x(), scene.cam().pos().y(), scene.cam().pos().z(),
               scene.cam().dir().x(), scene.cam().dir().y(), scene.cam().dir().z(),
               scene.cam().fov());
        printf("  Resolution: %dx%d  SPP: %d  MaxBounce: %d  GP_medium: %s\n",
               fullTargetW, fullTargetH, fullTargetSpp,
               g_pathTracerSettings.max_bounce,
               g_pathTracerSettings.skip_medium ? "skipped" : "enabled");
        printf("==================\n");
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

        // Render FPS: count SPP pass completions (edge-triggered, no consume)
        static bool wasSppDone = false;
        bool isSppDone = controller.isSppPassDone();
        if (isSppDone && !wasSppDone) sppPassCount++;
        wasSppDone = isSppDone;
        sppTimer += dt;
        if (sppTimer >= 0.5f) {
            renderFps = (sppTimer > 0.0f) ? sppPassCount / sppTimer : 0.0f;
            sppPassCount = 0;
            sppTimer = 0.0f;
        }
        if (saveToastTimer > 0.0f) saveToastTimer -= dt;

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
            std::string fname;
            if (activeTab >= 0 && activeTab < (int)savedImages.size()) {
                auto& img = savedImages[activeTab];
                fname = img.title + ".png";
                for (auto& c : fname) if (c == ':' || c == ' ') c = '_';
                Framebuffer fb(img.w, img.h);
                for (int y = 0; y < img.h; y++)
                    for (int x = 0; x < img.w; x++) {
                        int i = (y * img.w + x) * 3;
                        fb(x, y).rgb = Vec3f(img.pixels[i+0], img.pixels[i+1], img.pixels[i+2]);
                    }
                fb.save(fname);
            } else {
                // Save current live preview or full-render in-progress
                int w = isPreview ? previewW : fullTargetW;
                int h = isPreview ? previewH : fullTargetH;
                fname = "live_" + std::to_string(int(glfwGetTime())) + ".png";
                std::vector<float> raw(w * h * 3);
                controller.copyDisplayRawTo(raw.data(), w, h);
                Framebuffer fb(w, h);
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++) {
                        int i = (y * w + x) * 3;
                        fb(x, y).rgb = Vec3f(raw[i+0], raw[i+1], raw[i+2]);
                    }
                fb.save(fname);
            }
            saveToastTimer = 1.5f;
            saveToastPath  = fname;
        }
        ctrlSWasDown = ctrlSNow;

        // --- Ctrl+W → close current tab (Live unaffected) ---
        static bool ctrlWWasDown = false;
        bool wKeyDown = glfwGetKey(viewer.window(), GLFW_KEY_W) == GLFW_PRESS;
        bool ctrlWNow = ctrlDown && !wKeyDown && ctrlWWasDown; // detect ctrl+W release (W up while ctrl still down, or detect edge)
        // Simpler: edge-detect ctrl+W press
        if (ctrlDown && wKeyDown && !ctrlWWasDown) {
            if (activeTab >= 0 && activeTab < (int)savedImages.size()) {
                savedImages[activeTab].open = false;
            }
        }
        ctrlWWasDown = (ctrlDown && wKeyDown);

        // --- T / Shift+T → cycle tabs ---
        bool shiftDown = glfwGetKey(viewer.window(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
                      || glfwGetKey(viewer.window(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        static bool tWasDown = false;
        bool tDown = glfwGetKey(viewer.window(), GLFW_KEY_T) == GLFW_PRESS && !ctrlDown;
        if (tDown && !tWasDown) {
            int dir = shiftDown ? -1 : 1;
            activeTab += dir;
            if (activeTab >= (int)savedImages.size()) activeTab = -1;
            else if (activeTab < -1) activeTab = (int)savedImages.size() - 1;
        }
        tWasDown = tDown;

        // --- Q → quit ---
        if (glfwGetKey(viewer.window(), GLFW_KEY_Q) == GLFW_PRESS && !ctrlDown) {
            glfwSetWindowShouldClose(viewer.window(), GLFW_TRUE);
        }

        // --- When full render completes, capture result to tab ---
        if (!isPreview && controller.isSppPassDone() &&
            controller.getCurrentSpp() >= fullTargetSpp) {
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
            texBuf.resize(img.w * img.h * 3);
            // SavedImage stores raw HDR — tonemap for display
            for (int i = 0; i < img.w * img.h * 3; i += 3) {
                float r = img.pixels[i+0], g = img.pixels[i+1], b = img.pixels[i+2];
                texBuf[i+0] = r / (1.0f + r);
                texBuf[i+1] = g / (1.0f + g);
                texBuf[i+2] = b / (1.0f + b);
            }
            viewer.uploadTexture(texBuf.data(), texW, texH);
        } else {
            texBuf.resize(previewW * previewH * 3);
            if (isPreview) {
                controller.copyDisplayTo(texBuf.data(), previewW, previewH);
            }
            viewer.uploadTexture(texBuf.data(), previewW, previewH);
        }

        // --- Dynamic layout: use logical window size for ImGui coords ---
        int winW, winH;
        glfwGetWindowSize(viewer.window(), &winW, &winH);
        float panelW = float(winW) * 0.25f;
        float viewW  = float(winW) - panelW;
        float viewH  = float(winH);

        // --- Settings panel (right 1/4) ---
        ImGui::SetNextWindowPos(ImVec2(viewW, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelW, viewH), ImGuiCond_Always);
        ImGui::Begin("Settings", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        if (activeTab < 0) {
            // --- Live preview settings ---
            ImGui::Text("Render FPS: %.0f", renderFps);
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
                if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
                    controller.cancel();
                    launchPreview();
                }
            }

            ImGui::Separator();

            int bounce = g_pathTracerSettings.max_bounce;
            if (ImGui::SliderInt("Max Bounce", &bounce, 1, 20)) {
                g_pathTracerSettings.max_bounce = bounce;
                if (isPreview) launchPreview();
            }
            if (ImGui::SliderInt("Preview SPP", &previewSpp, 1, 16)) {
                if (isPreview) launchPreview();
            }
            ImGui::InputInt("Preview W", &previewW, 1, 100);
            ImGui::InputInt("Preview H", &previewH, 1, 100);
            int fs = fullSpp;
            if (ImGui::SliderInt("Full SPP", &fs, 1, 256))
                fullSpp = fs;
            ImGui::InputInt("Target W", &targetW, 1, 100);
            ImGui::InputInt("Target H", &targetH, 1, 100);

            ImGui::Separator();
            if (controller.hasGpMedium()) {
                if (ImGui::CollapsingHeader("GP Medium")) {
                    static const char* modes[] = {"single_realization", "renewal_plus"};
                    int modeIdx = (controller.gpMode() == "renewal_plus") ? 1 : 0;
                    if (ImGui::Combo("Mode", &modeIdx, modes, 2)) {
                        controller.setGpMode(modes[modeIdx]);
                        if (isPreview) launchPreview();
                    }
                    float s = controller.gpKernelSigma();
                    if (ImGui::InputFloat("Sigma", &s, 0.1f, 1.0f, "%.1f")) {
                        controller.setGpKernelSigma(s);
                    }
                    float l = controller.gpKernelLength();
                    if (ImGui::InputFloat("Length", &l, 0.1f, 1.0f, "%.1f")) {
                        controller.setGpKernelLength(l);
                    }
                    int p = controller.gpPointsPerCell();
                    if (ImGui::InputInt("Pts/Cell", &p, 1, 10)) {
                        controller.setGpPointsPerCell(p);
                    }
                }
            }
            ImGui::Text("Camera: %.0f %.0f %.0f",
                scene.cam().pos().x(), scene.cam().pos().y(), scene.cam().pos().z());
            ImGui::Text("RMB=look  WASD=move  SPACE=render");
        } else {
            // --- Saved image tab ---
            if (activeTab < (int)savedImages.size()) {
                auto& si = savedImages[activeTab];
                ImGui::Text("%s", si.title.c_str());
                ImGui::Text("%dx%d | %d SPP", si.w, si.h, si.spp);
                ImGui::Separator();
                if (ImGui::Button("Save PNG", ImVec2(-1, 0))) {
                    std::string fname = si.title + ".png";
                    for (auto& c : fname) if (c == ':' || c == ' ') c = '_';
                    Framebuffer fb(si.w, si.h);
                    for (int y = 0; y < si.h; y++)
                        for (int x = 0; x < si.w; x++) {
                            int i = (y * si.w + x) * 3;
                            fb(x, y).rgb = Vec3f(si.pixels[i+0], si.pixels[i+1], si.pixels[i+2]);
                        }
                    fb.save(fname);
                    saveToastTimer = 1.5f;
                    saveToastPath  = fname;
                }
                ImGui::Text("(or Ctrl+S)");
                ImGui::Separator();
                if (ImGui::Button("Save Config", ImVec2(-1, 0))) {
                    std::string cfgPath = controller.configPath();
                    std::ifstream in(cfgPath);
                    if (in.is_open()) {
                        json j = json::parse(in);
                        j["camera"]["pos"][0] = scene.cam().pos().x();
                        j["camera"]["pos"][1] = scene.cam().pos().y();
                        j["camera"]["pos"][2] = scene.cam().pos().z();
                        j["camera"]["dir"][0] = scene.cam().dir().x();
                        j["camera"]["dir"][1] = scene.cam().dir().y();
                        j["camera"]["dir"][2] = scene.cam().dir().z();
                        j["width"]  = si.w;
                        j["height"] = si.h;
                        j["spp"]    = si.spp;
                        j["max_bounce"] = g_pathTracerSettings.max_bounce;
                        j["gp_mode"] = controller.gpMode();
                        if (controller.hasGpMedium()) {
                            if (!j.contains("kernel")) j["kernel"] = json::object();
                            j["kernel"]["sigma"]         = controller.gpKernelSigma();
                            j["kernel"]["length_scale"]  = controller.gpKernelLength();
                            j["kernel"]["points_per_cell"] = controller.gpPointsPerCell();
                        }
                        std::string outName = "config_" + si.title + ".json";
                        for (auto& c : outName) if (c == ':' || c == ' ') c = '_';
                        std::ofstream out(outName);
                        out << j.dump(2);
                        saveToastTimer = 1.5f;
                        saveToastPath  = outName;
                    }
                }
            }
        }
        if (saveToastTimer > 0.0f) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Saved %s",
                              saveToastPath.c_str());
        }
        ImGui::End();

        // --- Viewport with tabs ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(viewW, viewH));
        ImGui::Begin("Viewport", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // --- Manual tab bar ---
        ImGui::BeginGroup();
        {
            ImGui::PushStyleColor(ImGuiCol_Button, activeTab == -1
                ? ImGui::GetStyleColorVec4(ImGuiCol_TabActive)
                : ImGui::GetStyleColorVec4(ImGuiCol_Tab));
            if (ImGui::Button("Live")) activeTab = -1;
            ImGui::PopStyleColor();
        }
        for (int i = 0; i < (int)savedImages.size(); i++) {
            ImGui::SameLine(0, 2);
            ImGui::PushStyleColor(ImGuiCol_Button, activeTab == i
                ? ImGui::GetStyleColorVec4(ImGuiCol_TabActive)
                : ImGui::GetStyleColorVec4(ImGuiCol_Tab));
            if (ImGui::Button(savedImages[i].title.c_str())) activeTab = i;
            ImGui::PopStyleColor();
            // right-click to close
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                savedImages[i].open = false;
        }
        savedImages.erase(
            std::remove_if(savedImages.begin(), savedImages.end(),
                [](const SavedImage& s) { return !s.open; }),
            savedImages.end());
        if (activeTab >= (int)savedImages.size()) activeTab = (int)savedImages.size() - 1;
        if (savedImages.empty()) activeTab = -1;
        ImGui::EndGroup();

        // Scale and draw texture in remaining content area
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float scale = std::min(avail.x / float(texW), avail.y / float(texH));
        float imgW  = float(texW) * scale;
        float imgH  = float(texH) * scale;
        float padX  = (avail.x - imgW) * 0.5f;
        float padY  = (avail.y - imgH) * 0.5f;
        if (padX < 0) padX = 0; if (padY < 0) padY = 0;
        if (imgW > avail.x) { imgW = avail.x; imgH = imgW * float(texH) / float(texW); }

        ImGui::SetCursorPos(
            ImVec2(ImGui::GetCursorPosX() + padX,
                   ImGui::GetCursorPosY() + padY));
        ImGui::Image((ImTextureID)(intptr_t)viewer.texID(), ImVec2(imgW, imgH),
                     ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();

        viewer.endFrame();
    }

    controller.stop();
    return 0;
}

} // namespace mupsi
