#include "controller.h"
#include "geometry/scene.h"
#include "rendering/renderer.h"
#include "rendering/trace.h"
#include "rendering/framebuffer.h"
#include "geometry/primitive.h"
#include "primitives/sphere.h"
#include "primitives/skydrome.h"
#include "bsdf/bsdf.h"
#include "texture/texture.h"
#include "medium/gpmedium.h"
#include "gp/gpnoise.h"
#include "gp/meanfunction.h"
#include "rendering/camera.h"

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
    std::string gpMode = j.value("gp_mode", std::string("single_realization"));
    if (gpMode == "renewal_plus")
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

  stop(); // join previous thread if any

  shutdown_ = false;
  cancel_   = false;

  renderThread_ = std::thread([this]() {
    renderer_ = std::make_shared<Renderer>();
    renderer_->setCancelFlag(&cancel_);
    renderer_->prepareRender(*scene_);
    renderer_->startRender(*scene_, spp_);
    if (!cancel_.load()) {
      framebufferFront_ = renderer_->getFramebuffer();
      frameReady_.store(true);
    }
  });
}

void Controller::startProgressive(int targetSpp,
    std::function<void(const Framebuffer&, int)> onFrame) {
  if (!scene_) return;
  stop();
  shutdown_ = false;
  cancel_   = false;

  renderThread_ = std::thread([this, targetSpp, onFrame = std::move(onFrame)]() {
    renderer_ = std::make_shared<Renderer>();
    renderer_->setCancelFlag(&cancel_);
    renderer_->prepareRender(*scene_);
    renderer_->startRenderProgressive(*scene_, targetSpp,
        [&](int s) {
          if (!cancel_.load()) onFrame(*renderer_->getFramebuffer(), s);
        });
    if (!cancel_.load()) {
      framebufferFront_ = renderer_->getFramebuffer();
      frameReady_.store(true);
    }
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

bool Controller::isFrameReady() const {
  return frameReady_.load();
}

const Framebuffer& Controller::getFrameBuffer() const {
  return *framebufferFront_;
}

void Controller::consumeFrameBuffer() {
  frameReady_.store(false);
}

} // namespace mupsi
