#ifndef _BSDF_H_
#define _BSDF_H_

#include <Eigen/Core>
using namespace Eigen;

namespace mupsi {

class Sampler; 

struct SurfaceScatterEvent{
  
  // input 
    Vector3f wo; 
    Vector3f normal; 
    Sampler* sampler; 

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

class Bsdf {

  public: 

    virtual ~Bsdf() = default; 
    Bsdf() = default;

    virtual void eval(SurfaceScatterEvent& event) const = 0; // calculate radience 
    virtual void sample(SurfaceScatterEvent& event) const = 0;  // pick a wi randomly
    virtual void pdf(SurfaceScatterEvent& event) const = 0;  // wi, wo -> pdf
}; 

class LambertianBsdf: public Bsdf {

  public: 

    LambertianBsdf(): albedo_(Vector3f(0.8, 0.8, 0.8)) {}; 
    LambertianBsdf(const Vector3f& albedo): albedo_(albedo) {};
    virtual ~LambertianBsdf() = default; 

    void eval(SurfaceScatterEvent& event) const override; 
    void sample(SurfaceScatterEvent& event) const override;  
    void pdf(SurfaceScatterEvent& event) const override;  

  private:
    Vector3f albedo_;
};
}

#endif 