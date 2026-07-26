#include "gpmedium.h"
#include "geometry/ray.h"
#include "gp/meanfunction.h"
#include "math/sampler.h"

namespace mupsi {


  float GPMedium::eval(const Vector3f& p, uint32_t seed) const {
    float mean = mean_->eval(p);
    float noise = noiseGenerator_->RawNoise(p, seed);
    return mean + noise;
  }

  float GPMedium::eval(const Vector3f& p, uint32_t seed, GPConditioningState& state) const {
    float f = eval(p, seed); 
    if (state.active) {
      float hcp = state.u_tilde * noiseGenerator_->getKernel()->h(state.C, p); 
      float hcp_grad = state.g_tilde.dot(noiseGenerator_->getKernel()->h_grad(state.C, p));
      return f + hcp + hcp_grad;
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
        sample.t = t; 
        sample.p = ray.origin() + ray.direction() * t; 
        sample.normal =  sampleGradient(sample.p, medium_sampler);
        sample.bsdf = bsdf_.get();

        /* fill: 
          struct GPConditioningState {
            bool active = false;
            Vector3f C = Vector3f::Zero();
            float u_tilde = 0.0f;
            Vector3f gradient_scale = Vector3f::Zero();
        };
        */
       sample.conditioning->active = false;
       if (g_gpSettings.gpMode == GPSettings::GPCorrelationMode::RenewalPlus) {

        sample.conditioning->active = true;
        sample.conditioning->C = sample.p; 
        /*
          $$\boxed{\tilde{u} = -\frac{\mu(\mathbf{C})}{A} -
          \psi_{\text{raw}}(\mathbf{C})}$$
        */
        sample.conditioning->u_tilde = -evalMu(sample.p) / noiseGenerator_->getKernel()->getSigma() 
                - evalPsi(sample.p, seed);
        /*
          g = -\frac{L^2}{2} \cdot (\text{targetGrad} - \nabla\mu(C) -  
          \nabla\psi(C))
        */
        sample.conditioning->g_tilde = 
          noiseGenerator_->getKernel()->oneOverSecondDerivative() * 
                (
                  sample.normal - 
                  muGradient(sample.p) - psiGradient(sample.p, seed)
                ); 
        }
              
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
  
  

  Vector3f GPMedium::sampleGradient(const Vector3f& p, Sampler& medium_sampler) const {
    uint32_t seed = medium_sampler.nextI();
    float eps = g_gpSettings.gpeps;  // TODO: hard-coded value
    Vector3f normal;
    normal.x() = eval(p + Vector3f(eps, 0, 0), seed) - eval(p - Vector3f(eps, 0, 0), seed);
    normal.y() = eval(p + Vector3f(0, eps, 0), seed) - eval(p - Vector3f(0, eps, 0), seed);
    normal.z() = eval(p + Vector3f(0, 0, eps), seed) - eval(p - Vector3f(0, 0, eps), seed);
    return normal.normalized();

  }
}