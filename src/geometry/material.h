#ifndef _MATERIAL_H_ 
#define _MATERIAL_H_

#include <Eigen/Core> 

using namespace Eigen; 

namespace mupsi {


class Material {
public:
  Material(const Vector3f& Ka, bool hasEmit, float emit=0.0f): Ka(Ka), hasEmit(hasEmit), emission(emit) {};
  bool hasEmmision() const { return hasEmit; };
  bool getEmission() const { return emission; };

  void bsdf (const Vector3f& wo, const Vector3f& normal, Vector3f& wi, float& pdf); 
  Vector3f evalRadiance(const Vector3f& wo, const Vector3f& wi, const Vector3f& normal); 

private: 
  Vector3f Ka; 
  bool hasEmit;
  float emission; 

  Vector3f sampleHemisphere() const; 
  Vector3f toWorld(const Vector3f& vec, const Vector3f normal) const; 



}; 

extern Material g_defaultMaterial;


}

#endif 