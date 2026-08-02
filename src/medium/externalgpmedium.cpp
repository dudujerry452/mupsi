#include "externalgpmedium.h"
#include "geometry/ray.h"
#include "math/sampler.h"
#include <cmath>
#include <cstdint>

namespace mupsi {

static float hash11(float x, float y, float z, uint32_t seed) {
  uint32_t h = seed;
  h = h * 0x9e3779b1u + *reinterpret_cast<const uint32_t*>(&x);
  h = h * 0x9e3779b1u + *reinterpret_cast<const uint32_t*>(&y);
  h = h * 0x9e3779b1u + *reinterpret_cast<const uint32_t*>(&z);
  h = (h ^ (h >> 16)) * 0x85ebca6bu;
  h = (h ^ (h >> 13)) * 0xc2b2ae35u;
  h = h ^ (h >> 16);
  return (float)(h & 0xFFFFFFu) / (float)0xFFFFFFu * 2.0f - 1.0f;
}

float ExternalGPMedium::hashNoise(const Vector3f& p, uint32_t seed) const {
  return hash11(p.x(), p.y(), p.z(), seed);
}

float ExternalGPMedium::eval(const Vector3f& p, uint32_t seed) const {
  return evalWithGradient(p, seed).x();
}

float ExternalGPMedium::eval(const Vector3f& p, uint32_t seed, GPConditioningState& state) const {
  (void)state;
  return eval(p, seed);
}

Vector4f ExternalGPMedium::evalWithGradient(const Vector3f& p, uint32_t seed) const {
  float mu = mean_->eval(p);
  Vector3f mu_grad = mean_->gradient(p);

  float noise = 0.0f;
  Vector3f noise_grad = Vector3f::Zero();
  if (variance_ && noiseScale_ > 0.0f) {
    float var = variance_->eval(p);
    if (var > 0.0f) {
      float amp = noiseScale_ * std::sqrt(var);
      noise = amp * hashNoise(p, seed);
      float eps = 0.01f;
      float nx = amp * (hashNoise(Vector3f(p.x() + eps, p.y(), p.z()), seed)
                      - hashNoise(Vector3f(p.x() - eps, p.y(), p.z()), seed)) / (2.0f * eps);
      float ny = amp * (hashNoise(Vector3f(p.x(), p.y() + eps, p.z()), seed)
                      - hashNoise(Vector3f(p.x(), p.y() - eps, p.z()), seed)) / (2.0f * eps);
      float nz = amp * (hashNoise(Vector3f(p.x(), p.y(), p.z() + eps), seed)
                      - hashNoise(Vector3f(p.x(), p.y(), p.z() - eps), seed)) / (2.0f * eps);
      noise_grad = Vector3f(nx, ny, nz);
    }
  }
  return Vector4f(mu + noise,
                  mu_grad.x() + noise_grad.x(),
                  mu_grad.y() + noise_grad.y(),
                  mu_grad.z() + noise_grad.z());
}

Vector4f ExternalGPMedium::evalWithGradient(const Vector3f& p, uint32_t seed, GPConditioningState& state) const {
  (void)state;
  return evalWithGradient(p, seed);
}

inline bool sign(float t) { return t > 0.0f; }

// sphere-tracing style ray marching: step = max(|SDF|, min_step)
bool ExternalGPMedium::sampleDistance(Ray& ray, MediumSample& sample, Sampler& medium_sampler) const {
  float nt = ray.nearT(), ft = ray.farT();
  ft = std::min(ft, 2000.0f);
  const float min_step = 0.005f;
  const int max_iter = 1024;
  uint32_t seed = medium_sampler.nextI();

  float t = nt;
  float f_prev = eval(ray.origin(), seed, *sample.conditioning);
  for (int iter = 0; iter < max_iter && t < ft; ++iter) {
    Vector3f p = ray.origin() + ray.direction() * t;
    float f_c = eval(p, seed, *sample.conditioning);

    // Sign change detection
    if (sign(f_c) != sign(f_prev) && std::fabs(f_prev - f_c) > 1e-6f) {
      // Bisection refinement
      float lo = t - std::max(std::fabs(f_prev), min_step);
      float hi = t;
      for (int b = 0; b < 16; ++b) {
        float mid = (lo + hi) * 0.5f;
        float f_mid = eval(ray.origin() + ray.direction() * mid, seed, *sample.conditioning);
        if (sign(f_mid) == sign(f_prev)) lo = mid;
        else hi = mid;
      }
      sample.exited = false;
      sample.t = hi;
      sample.p = ray.origin() + ray.direction() * hi;
      Vector4f f = evalWithGradient(sample.p, seed, *sample.conditioning);
      sample.normal = f.tail<3>().normalized();
      sample.bsdf = bsdf_.get();
      return true;
    }

    // Sphere tracing step
    float step = std::max(std::fabs(f_c), min_step);
    f_prev = f_c;
    t += step;
  }
  sample.exited = true;
  return false;
}

Vector3f ExternalGPMedium::transmittance(const Ray& ray, MediumSample& sample, Sampler& medium_sampler) const {
  float nt = ray.nearT(), ft = ray.farT();
  ft = std::min(ft, 2000.0f);
  const float min_step = 0.005f;
  const int max_iter = 1024;
  uint32_t seed = medium_sampler.nextI();

  float t = nt;
  float f_prev = eval(ray.origin(), seed, *sample.conditioning);
  for (int iter = 0; iter < max_iter && t < ft; ++iter) {
    Vector3f p = ray.origin() + ray.direction() * t;
    float f_c = eval(p, seed, *sample.conditioning);

    if (sign(f_c) != sign(f_prev) && std::fabs(f_prev - f_c) > 1e-6f) {
      return Vector3f::Zero();
    }
    f_prev = f_c;
    t += std::max(std::fabs(f_c), min_step);
  }
  return Vector3f::Ones();
}

} // namespace mupsi
