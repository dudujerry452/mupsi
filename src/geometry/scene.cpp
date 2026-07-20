#include "scene.h"
#include <memory>
#include <iostream>
#include <utility>
#include "gp/gpnoise.h"
#include "rendering/trace.h"

using namespace mupsi;

namespace mupsi {

// Thread-local conditioning state — one copy per OpenMP thread.
// Eliminates data races on the shared GPScene instance.
thread_local uint32_t g_cond_seed;

} // namespace mupsi

namespace {
    thread_local ConditioningState tls_cond;
    thread_local ConditioningState tls_cond_next;
    thread_local uint32_t tls_cond_seed_next;
}

SDFScene::SDFScene()
{
}

void SDFScene::add(std::unique_ptr<SDFObject> obj)
{
  objects.push_back(std::move(obj));
  bounding = bounding + objects.back()->getBounding();
}

float SDFScene::eval(const Vector3f &pos, const SDFObject*& obj) const
{
  float min_distance = std::numeric_limits<float>::max();
  for (const auto &obj_ptr : objects)
  {
    float eval_value = obj_ptr->eval(pos);
    if(eval_value < min_distance) {
      min_distance = eval_value;
      obj = obj_ptr.get();
    }
  }
  return min_distance;
}

Vector3f SDFScene::getNormal(const Vector3f &pos) const {

  float eps = g_rayTraceConfig.eps;  // small value for numerical differentiation
  Vector3f normal;
  normal.x() = eval(pos + Vector3f(eps, 0, 0)) - eval(pos - Vector3f(eps, 0, 0));
  normal.y() = eval(pos + Vector3f(0, eps, 0)) - eval(pos - Vector3f(0, eps, 0));
  normal.z() = eval(pos + Vector3f(0, 0, eps)) - eval(pos - Vector3f(0, 0, eps));
  return normal.normalized();
}

float GPScene::eval(const Vector3f &pos, const SDFObject*& obj) const
{
  float mu = SDFScene::eval(pos, obj);
  float psi = gpnoise.Noise(pos, g_cond_seed);
  if (tls_cond.active) {
    const auto& k = gpnoise.getKernel();
    psi += k.h(tls_cond.C, pos) * tls_cond.u_tilde
         + k.h_grad(tls_cond.C, pos).dot(tls_cond.gradient_scale);
  }
  return mu + psi;
}

void GPScene::prepareConditioning(const Vector3f& C, const Vector3f& targetGrad, uint32_t nextSeed)
{
  const SDFObject* dummy = nullptr;
  float mu_C = SDFScene::eval(C, dummy);

  // Use Noise (not RawNoise) — same normalisation as GPScene::eval
  float psi_C = gpnoise.Noise(C, nextSeed);
  tls_cond_next.u_tilde = -mu_C - psi_C;

  // numerical gradient of psi (Noise) at C
  float eps = g_rayTraceConfig.eps;
  Vector3f grad_psi_C;
  grad_psi_C.x() = gpnoise.Noise(C + Vector3f(eps, 0, 0), nextSeed)
                 - gpnoise.Noise(C - Vector3f(eps, 0, 0), nextSeed);
  grad_psi_C.y() = gpnoise.Noise(C + Vector3f(0, eps, 0), nextSeed)
                 - gpnoise.Noise(C - Vector3f(0, eps, 0), nextSeed);
  grad_psi_C.z() = gpnoise.Noise(C + Vector3f(0, 0, eps), nextSeed)
                 - gpnoise.Noise(C - Vector3f(0, 0, eps), nextSeed);
  grad_psi_C /= (2.0f * eps);

  // ∇mu(C) using base SDF (non-virtual dispatch against SDFScene)
  Vector3f grad_mu_C;
  grad_mu_C.x() = SDFScene::eval(C + Vector3f(eps, 0, 0), dummy)
                - SDFScene::eval(C - Vector3f(eps, 0, 0), dummy);
  grad_mu_C.y() = SDFScene::eval(C + Vector3f(0, eps, 0), dummy)
                - SDFScene::eval(C - Vector3f(0, eps, 0), dummy);
  grad_mu_C.z() = SDFScene::eval(C + Vector3f(0, 0, eps), dummy)
                - SDFScene::eval(C - Vector3f(0, 0, eps), dummy);

  Vector3f delta = targetGrad - grad_mu_C - grad_psi_C;
  tls_cond_next.gradient_scale = gpnoise.getKernel().oneOverSecondDerivative() * delta;

  tls_cond_next.C = C;
  tls_cond_next.active = true;
  tls_cond_seed_next = nextSeed;
}

std::pair<ConditioningState, uint32_t> GPScene::activateNextConditioning()
{
  auto old = std::make_pair(tls_cond, g_cond_seed);
  tls_cond = tls_cond_next;
  g_cond_seed = tls_cond_seed_next;
  return old;
}

void GPScene::restoreConditioning(const ConditioningState& cond, uint32_t seed)
{
  tls_cond = cond;
  g_cond_seed = seed;
}

