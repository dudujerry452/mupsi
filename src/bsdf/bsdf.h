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
    Vector3f rad; // radiance

    SurfaceScatterEvent() = default;
}; 

struct LightSample {
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

    virtual void eval(SurfaceScatterEvent& event) const = 0; // calculate radience 
    virtual void sample(SurfaceScatterEvent& event) const = 0;  // pick a wi randomly
    virtual void pdf(SurfaceScatterEvent& event) const = 0;  // wi, wo -> pdf
}; 

class LambertianBsdf: public Bsdf {

  public: 

    LambertianBsdf(): albedo_(Bsdf::default_albedo_) {}; 
    LambertianBsdf(std::shared_ptr<Texture> albedo): albedo_(albedo) {};
    virtual ~LambertianBsdf() = default; 

    void eval(SurfaceScatterEvent& event) const override; 
    void sample(SurfaceScatterEvent& event) const override;  
    void pdf(SurfaceScatterEvent& event) const override;  

  private:
    std::shared_ptr<Texture> albedo_;
};
}

#endif 