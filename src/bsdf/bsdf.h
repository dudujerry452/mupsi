#ifndef _BSDF_H_
#define _BSDF_H_

#include <Eigen/Core>
#include <memory>
#include "geometry/intersection.h"
using namespace Eigen;

namespace mupsi {

class Sampler; 

struct SurfaceScatterEvent{
  
  // input 
    Vector3f wo; 
    Vector3f normal; 
    Sampler* sampler; 
    const IntersectionInfo* info; 

  // output 
    Vector3f wi; 
    float pdf; 
    Vector3f weight; // 反射率, albedo

    SurfaceScatterEvent() = default;
}; 

struct LightSample {
  Vector3f weight; 
  Vector3f d; // shade point -> sample point, normalized
  float dist; 
  float pdf; 
};

class Texture; 

class Bsdf {

  protected: 
    static std::shared_ptr<Texture> default_albedo_; 

  public: 

    virtual ~Bsdf() = default; 
    Bsdf() = default;

    virtual Vector3f eval(SurfaceScatterEvent& event) const = 0; // calculate radience 
    virtual float pdf(SurfaceScatterEvent& event) const = 0;  // wi, wo -> pdf

    Vector3f weight(SurfaceScatterEvent& event) const {
      float pdf_val = pdf(event); 
      if(pdf_val != 0.0f) return eval(event) / pdf_val; 
      else  return Vector3f(0.0f, 0.0f, 0.0f); 
    }

    virtual void sample(SurfaceScatterEvent& event) const = 0;  // pick a wi randomly, and fill pdf, weight
    
}; 

class LambertianBsdf: public Bsdf {

  public: 

    LambertianBsdf(): albedo_(Bsdf::default_albedo_) {}; 
    LambertianBsdf(std::shared_ptr<Texture> albedo): albedo_(albedo) {};
    virtual ~LambertianBsdf() = default; 

    Vector3f eval(SurfaceScatterEvent& event) const override; 
    float pdf(SurfaceScatterEvent& event) const override;  
    void sample(SurfaceScatterEvent& event) const override;  

  private:
    std::shared_ptr<Texture> albedo_;
};


class NullBsdf: public Bsdf {
  public: 
    NullBsdf() = default; 
    virtual ~NullBsdf() = default; 

    Vector3f eval(SurfaceScatterEvent& event) const override; 
    float pdf(SurfaceScatterEvent& event) const override;  
    void sample(SurfaceScatterEvent& event) const override;   
}; 
}

#endif 