#include "controller.h"
#include "geometry/scene.h"
#include "rendering/renderer.h"
#include "rendering/render_context.h"
#include "rendering/trace.h"
#include "rendering/framebuffer.h"
#include "geometry/primitive.h"
#include "primitives/sphere.h"
#include "primitives/mesh.h"
#include "primitives/skydrome.h"
#include "bsdf/bsdf.h"
#include "texture/texture.h"
#include "medium/gpmedium.h"
#include "gp/gpnoise.h"
#include "gp/meanfunction.h"
#include "rendering/camera.h"

#include <Eigen/Geometry>
#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <iostream>

using json = nlohmann::json;

namespace mupsi {

// --- helpers ---

static Vector3f jsonToVec3(const json& arr) {
  return Vector3f(arr.at(0).get<float>(),
                  arr.at(1).get<float>(),
                  arr.at(2).get<float>());
}

// --- load ---

bool Controller::load(std::string config_path) {
  configPath_ = config_path;
  hasGpMedium_ = false;
  std::ifstream f(config_path);
  if (!f.is_open()) {
    std::cerr << "Controller::load failed to open " << config_path << std::endl;
    return false;
  }
  json j = json::parse(f);

  // --- Path tracer settings ---
  if (j.contains("max_bounce"))
    g_pathTracerSettings.max_bounce = j["max_bounce"];
  if (j.contains("max_medium_bounce"))
    g_pathTracerSettings.max_medium_bounce = j["max_medium_bounce"];
  if (j.contains("spp"))
    spp_ = j["spp"];

  int w = j.value("width", 256);
  int h = j.value("height", 256);
  outputPath_ = j.value("output", std::string("test.png"));

  // --- Camera ---
  const auto& cam = j["camera"];
  Vector3f camPos  = jsonToVec3(cam["pos"]);
  Vector3f camDir  = jsonToVec3(cam["dir"]);
  Vector3f camUp   = jsonToVec3(cam.value("up", std::vector<float>{0,1,0}));
  float     camFov = cam.value("fov", 45.0f);

  scene_ = std::make_shared<Scene>();
  scene_->setCamera(std::make_shared<Camera>(camPos, camDir, camUp, camFov, w, h));

  // --- GP settings ---
  {
    gpMode_ = j.value("gp_mode", std::string("single_realization"));
    if (gpMode_ == "renewal_plus")
      g_gpSettings.gpMode = GPSettings::GPCorrelationMode::RenewalPlus;
    else
      g_gpSettings.gpMode = GPSettings::GPCorrelationMode::SingleRealization;
  }

  // --- GP medium ---
  std::shared_ptr<GPMedium> gpmedium;
  if (j.contains("gp_medium")) {
    const auto& gpm = j["gp_medium"];

    // Mean function
    std::shared_ptr<MeanFunction> mean;
    std::string meanType = gpm.value("mean_type", std::string("sphere"));
    if (meanType == "sphere") {
      Vector3f mc = jsonToVec3(gpm["mean_center"]);
      float     mr = gpm.value("mean_radius", 70.0f);
      mean = std::make_shared<SphereMeanFunction>(mc, mr);
    } else {
      std::cerr << "Controller::load unknown mean_type: " << meanType << std::endl;
      return false;
    }

    // Kernel
    const auto& kern = j.value("kernel", json::object());
    float sigma   = kern.value("sigma", 1.0f);
    float length  = kern.value("length_scale", 1.0f);
    Vector3f aniso = jsonToVec3(kern.value("aniso", std::vector<float>{1,1,1}));
    int ptsPerCell = kern.value("points_per_cell", 3);

    auto kernel = std::make_shared<SparseSEKernel>(sigma, length, aniso);
    auto noiseGen = std::make_shared<SparseGPNoiseGenerator>(kernel, ptsPerCell);

    gpmedium = std::make_shared<GPMedium>(mean, noiseGen);

    hasGpMedium_      = true;
    gpMeanType_       = meanType;
    gpMeanCenter_     = (meanType == "sphere") ? jsonToVec3(gpm["mean_center"]) : Vector3f::Zero();
    gpMeanRadius_     = gpm.value("mean_radius", 70.0f);
    gpKernelSigma_    = sigma;
    gpKernelLength_   = length;
    gpPointsPerCell_  = ptsPerCell;
  }

  // --- Skydrome ---
  if (j.contains("skydrome")) {
    auto skyTex = std::make_shared<BitmapTexture>(j["skydrome"].get<std::string>());
    scene_->setSkydrome(std::make_shared<Skydrome>(skyTex));
  }

  // --- Primitives ---
  // Build BSDF cache
  std::map<std::string, std::shared_ptr<Bsdf>> bsdfCache;
  bsdfCache["lambertian"] = std::make_shared<LambertianBsdf>();
  bsdfCache["null"]       = std::make_shared<NullBsdf>();
  bsdfCache["specular"]   = std::make_shared<SpecularBsdf>();

  auto getBsdf = [&](const std::string& key) -> std::shared_ptr<Bsdf> {
    if (auto it = bsdfCache.find(key); it != bsdfCache.end())
      return it->second;
    std::cerr << "Controller::load unknown BSDF: " << key << std::endl;
    return bsdfCache["lambertian"];
  };

  if (!j.contains("primitives") || !j["primitives"].is_array()) {
    std::cerr << "Controller::load no primitives array in config" << std::endl;
    return false;
  }

  for (const auto& pj : j["primitives"]) {
    std::string type = pj.value("type", std::string("sphere"));
    std::string bsdfName = pj.value("bsdf", std::string("lambertian"));

    if (type == "sphere") {
      Vector3f center = jsonToVec3(pj["center"]);
      float    radius = pj.value("radius", 100.0f);
      auto prim = std::make_shared<Sphere>(center, radius, getBsdf(bsdfName));

      if (pj.contains("emission")) {
        Vector3f em = jsonToVec3(pj["emission"]);
        prim->setEmission(std::make_shared<ConstantTexture>(em));
      }

      if (pj.contains("int_medium") && gpmedium) {
        prim->setMedium(gpmedium, nullptr);
      }

      scene_->addPrimitive(prim);

    } else if (type == "mesh") {
      std::shared_ptr<Bsdf> bsdf = getBsdf(bsdfName);
      if (pj.contains("texture") && bsdfName == "lambertian") {
        bsdf = std::make_shared<LambertianBsdf>(
          std::make_shared<BitmapTexture>(pj["texture"].get<std::string>()));
      }
      auto m = std::make_shared<Mesh>(bsdf);
      if (!m->fetchFrom(pj["file"].get<std::string>())) {
        std::cerr << "Controller::load failed to load mesh: " << pj["file"] << std::endl;
        return false;
      }
      if (pj.contains("transform")) {
        const auto& t = pj["transform"];
        Vector3f pos = t.contains("position") ? jsonToVec3(t["position"]) : Vector3f::Zero();
        float    scl = t.contains("scale") ? t["scale"].get<float>() : 1.0f;
        m->setTransform(Affine3f(Translation3f(pos) * Scaling(scl)).matrix());
      }
      scene_->addPrimitive(m);

    } else {
      std::cerr << "Controller::load unknown primitive type: " << type << std::endl;
    }
  }

  std::cout << "Controller::load: " << config_path << " ok" << std::endl;
  return true;
}

// --- render control ---

void Controller::start() {
  if (!scene_) {
    std::cerr << "Controller::start: no scene loaded" << std::endl;
    return;
  }

  stop();

  shutdown_ = false;
  cancel_   = false;

  renderThread_ = std::thread([this]() {
    // Snapshot all mutable state before rendering — no globals or scene->cam()
    // are touched from this point onward.
    RenderContext ctx(scene_->cam(), g_pathTracerSettings, g_gpSettings, scene_);

    renderer_ = std::make_shared<Renderer>();
    renderer_->setCancelFlag(&cancel_);
    renderer_->prepareRender(ctx);
    renderer_->startRender(ctx, spp_);
    if (!cancel_.load()) {
      framebufferFront_ = renderer_->getFramebuffer();
      sppPassDone_.store(true);
    }
  });
}

void Controller::startProgressive(int targetSpp, bool skipMedium) {
  if (!scene_) return;
  stop();
  shutdown_ = false;
  cancel_   = false;
  sppPassDone_.store(false);
  currentSpp_.store(0);
  g_pathTracerSettings.skip_medium = skipMedium;

  renderThread_ = std::thread([this, targetSpp]() {
    // Snapshot all mutable state before rendering begins.
    // From this point on, the render thread touches NO globals or shared mutable state.
    RenderContext ctx(scene_->cam(), g_pathTracerSettings, g_gpSettings, scene_);

    renderer_ = std::make_shared<Renderer>();
    renderer_->setCancelFlag(&cancel_);
    renderer_->prepareRender(ctx);

    // Rebuild GP medium if kernel params changed
    if (hasGpMedium_)
        applyGpMedium();

    // Keep old front buffer alive so display doesn't flicker black
    int w = ctx.camera.width(), h = ctx.camera.height();
    framebufferBack_ = std::make_shared<Framebuffer>(w, h);
    if (!framebufferFront_ || framebufferFront_->width() != w || framebufferFront_->height() != h)
        framebufferFront_ = std::make_shared<Framebuffer>(w, h);

    renderer_->startRenderProgressive(ctx, targetSpp,
        [&](int s) {
          if (cancel_.load()) return;
          // Deep copy renderer's averaged pixels → back buffer
          const Framebuffer& rfb = *renderer_->getFramebuffer();
          for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
              (*framebufferBack_)(x, y).rgb = rfb(x, y).rgb;
          // Swap front/back
          {
            std::lock_guard<std::mutex> lock(displayMtx_);
            std::swap(framebufferFront_, framebufferBack_);
          }
          currentSpp_.store(s);
          sppPassDone_.store(true);
        });
  });
}

void Controller::cancel() {
  cancel_.store(true);
}

void Controller::stop() {
  cancel();
  if (renderThread_.joinable())
    renderThread_.join();
  shutdown_ = false;
}

void Controller::saveDisplay(const std::string& path) const {
  std::lock_guard<std::mutex> lock(displayMtx_);
  if (framebufferFront_) framebufferFront_->save(path);
}

void Controller::copyDisplayRawTo(float* dst, int w, int h) const {
  std::lock_guard<std::mutex> lock(displayMtx_);
  if (!framebufferFront_) return;
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      const Vec3f& v = (*framebufferFront_)(x, y).rgb;
      int i = (y * w + x) * 3;
      dst[i + 0] = v.x();
      dst[i + 1] = v.y();
      dst[i + 2] = v.z();
    }
}

void Controller::copyDisplayTo(float* dst, int w, int h) const {
  std::lock_guard<std::mutex> lock(displayMtx_);
  if (!framebufferFront_) return;
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      Vec3f v = framebufferFront_->tonemapped(x, y);
      int i = (y * w + x) * 3;
      dst[i + 0] = v.x();
      dst[i + 1] = v.y();
      dst[i + 2] = v.z();
    }
}

void Controller::setGpMode(const std::string& v) {
  gpMode_ = v;
  if (v == "renewal_plus")
    g_gpSettings.gpMode = GPSettings::GPCorrelationMode::RenewalPlus;
  else
    g_gpSettings.gpMode = GPSettings::GPCorrelationMode::SingleRealization;
}

void Controller::applyGpMedium() {
  if (!scene_) return;
  std::shared_ptr<MeanFunction> mean;
  if (gpMeanType_ == "sphere") {
    mean = std::make_shared<SphereMeanFunction>(gpMeanCenter_, gpMeanRadius_);
  }
  if (!mean) return;

  auto kernel = std::make_shared<SparseSEKernel>(
      gpKernelSigma_, gpKernelLength_, Vector3f(1,1,1));
  auto noiseGen = std::make_shared<SparseGPNoiseGenerator>(
      kernel, gpPointsPerCell_);
  auto newMedium = std::make_shared<GPMedium>(mean, noiseGen);

  // Update all primitives that have an int_medium set
  for (auto& prim : scene_->primitives_) {
    if (prim->getIntMedium())
      prim->setMedium(newMedium, prim->getExtMedium());
  }
}

} // namespace mupsi
