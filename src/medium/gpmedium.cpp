#include "gpmedium.h"
#include "geometry/ray.h"
#include "gp/meanfunction.h"
#include "math/sampler.h"

#include <iostream>

namespace mupsi {


  float GPMedium::eval(const Vector3f& p, uint32_t seed) const {
    return evalWithGradient(p, seed).x();
  }

  float GPMedium::eval(const Vector3f& p, uint32_t seed, GPConditioningState& state) const {
    return evalWithGradient(p, seed, state).x();
  }

  Vector4f GPMedium::evalWithGradient(const Vector3f& p, uint32_t seed) const {
    float mean = mean_->eval(p);
    Vector3f mean_grad = mean_->gradient(p);
    Vector4f noise = noiseGenerator_->RawNoise(p, seed);
    return Vector4f(mean + noise.x(), mean_grad.x() + noise.y(), mean_grad.y() + noise.z(), mean_grad.z() + noise.w());
  }

  Vector4f GPMedium::evalWithGradient(const Vector3f& p, uint32_t seed, GPConditioningState& state) const {
    Vector4f f = evalWithGradient(p, seed);
    if (state.active) {
      auto k = noiseGenerator_->getKernel().get();
      Vector3f hg = k->h_grad(state.C, p);
      // value correction:  ũ·h(C,p) + g̃·∇h(C,p)
      f.x() += state.u_tilde * k->h(state.C, p)
             + state.g_tilde.dot(hg);
      // gradient correction:  ũ·∇h(C,p) + H_h(C,p)·g̃
      Vector3f H_g = k->h_hessian_vec(state.C, p, state.g_tilde);
      f.y() += state.u_tilde * hg.x() + H_g.x();
      f.z() += state.u_tilde * hg.y() + H_g.y();
      f.w() += state.u_tilde * hg.z() + H_g.z();
    }
    return f;
  }

  inline bool sign(float t) {return t > 0.0f;}

  // sampler must be ConstantSampler 
  bool GPMedium::sampleDistance(Ray& ray, MediumSample& sample, Sampler& medium_sampler) const
  {
    // assert(dynamic_cast<ConstantSampler*>(&sampler) != nullptr);

    float nt = ray.nearT(), ft = ray.farT();
    ft = std::min(ft, 2000.0f); 
    int min_depth = 100; // 固化到配置 
    float max_dt = 0.5; 
    float dt = std::min(max_dt, (ft - nt) / min_depth);
    uint32_t seed = medium_sampler.nextI();

    float t = nt;
    float f_c; 
    float f_prev = eval(ray.origin(), seed, *sample.conditioning);
    bool hit = false; 
    while (t < ft) {
      Vector3f p = ray.origin() + ray.direction() * t;
      f_c = eval(p, seed, *sample.conditioning); 

      // TODO: hard-coded value
      if(sign(f_c) != sign(f_prev) && std::fabs(f_prev - f_c) > 1e-6) {
        float factor = f_prev / (f_prev - f_c); 
        do {
          float t_test = std::lerp(t-dt, t, factor); 
          float f_test = eval(ray.origin() + ray.direction() * t_test, seed, *sample.conditioning);
          if(sign(f_test) == sign(f_prev))  break; 
          factor *= 0.9; 
        } while(factor >= 0.01); 
        hit = true; 
      }

      if(hit) {
        sample.exited = false; 
        sample.t = t; 
        sample.p = ray.origin() + ray.direction() * t; 
        Vector4f f = evalWithGradient(sample.p, seed, *sample.conditioning);
        sample.normal =  f.tail<3>().normalized();
        sample.bsdf = bsdf_.get();

        /* fill: 
          struct GPConditioningState {
            bool active = false;
            Vector3f C = Vector3f::Zero();
            float u_tilde = 0.0f;
            Vector3f gradient_scale = Vector3f::Zero();
        };
        */

        // std::cout << "medium hit at " << sample.t << std::endl; 
        return true; 
      }

      t += dt;

    }
    sample.exited = true;
    return false; 
  }

  Vector3f GPMedium::transmittance(const Ray& ray, MediumSample& sample, Sampler& medium_sampler) const {
    float nt = ray.nearT(), ft = ray.farT();
    ft = std::min(ft, 2000.0f); 
    int min_depth = 100; // 固化到配置 
    float max_dt = 0.5; 
    float dt = std::min(max_dt, (ft - nt) / min_depth);
    uint32_t seed = medium_sampler.nextI();

    float t = nt;
    float f_c; 
    float f_prev = eval(ray.origin(), seed, *sample.conditioning);
    while (t < ft) {
      Vector3f p = ray.origin() + ray.direction() * t;
      f_c = eval(p, seed, *sample.conditioning); 

      // TODO: hard-coded value
      if(sign(f_c) != sign(f_prev) && std::fabs(f_prev - f_c) > 1e-6) {
        return Vector3f::Zero(); // 全不透明
      }

      t += dt; 

    }
    return Vector3f::Ones(); // 能穿过
  }
  
  
  // call it after the current condition is consumed
  void GPMedium::sampleCondition(MediumSample& sample, Sampler& medium_sampler) const
  {
    uint32_t seed = medium_sampler.nextI();

    sample.conditioning->active = false;
    if (g_gpSettings.gpMode == GPSettings::GPCorrelationMode::RenewalPlus && sample.exited == false) {

      sample.conditioning->active = true;
      sample.conditioning->C = sample.p; 

      Vector4f psi = evalPsi(sample.p, seed);
      /*
        $$\boxed{\tilde{u} = -\frac{\mu(\mathbf{C})}{A} -
        \psi_{\text{raw}}(\mathbf{C})}$$
      */
      sample.conditioning->u_tilde = -evalMu(sample.p) / noiseGenerator_->getKernel()->getSigma()
              - psi.x();
      /*
        g = -\frac{L^2}{2} \cdot (\text{targetGrad} - \nabla\mu(C) -  
        \nabla\psi(C))
      */
      sample.conditioning->g_tilde = 
        noiseGenerator_->getKernel()->oneOverSecondDerivative() * 
              (
                sample.normal - 
                muGradient(sample.p) - psi.tail<3>()
              ); 
      }

  }
}