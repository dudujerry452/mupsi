#include "material.h"
#include "gp/gp_eval.h"
#include "math/random.h"

#include <Eigen/Geometry>
#include <iostream>

using namespace Eigen; 

namespace mupsi {

Material g_defaultMaterial(Vector3f(0.0f, 0.0f, 1.0f), false, 0.0f);

Vector3f Material::sampleHemisphere() const {
  Random rng(42);  // TODO: maybe another seed
  float x1 = rng.next1D(), x2 = rng.next1D(); 
  float z = std::fabs(1.0f - 2.0f * x1); // to [-1, 1]
  float r = std::sqrt(1.0 - z*z), phi = x2 * 2.0f * M_PI; 
  return Vector3f(r*std::cos(phi), r*std::sin(phi), z);
}

Vector3f Material::toWorld(const Vector3f& vec, const Vector3f normal) const{
  Vector3f B, C; 
  if(std::fabs(normal.x()) > std::fabs(normal.y())) { // prevent sqrt(0)
    float invLen = 1.0f / std::sqrt(normal.x()*normal.x() + normal.z()*normal.z());
    C = invLen * Vector3f(normal.z(), 0.0f, -normal.x()); 
  } else {
    float invLen = 1.0f / std::sqrt(normal.y()*normal.y() + normal.z()*normal.z()); 
    C = invLen * Vector3f(0.0f, normal.z(), -normal.y()); 
  }
  B = normal.cross(C);
  return vec.x() * B + vec.y() * C + vec.z() * normal; // is normalized
}

void Material::bsdf(const Vector3f& wo, const Vector3f& normal, Vector3f& wi, float& pdf) {
  wi = toWorld(sampleHemisphere(), normal);
  // wi = normal; 
  pdf = 0.5 / M_PI; // condition : wo . normal > 0.0f
} 

Vector3f Material::evalRadiance(const Vector3f& wo, const Vector3f& wi, const Vector3f& normal) {
  float cosalpha = wo.dot(normal); 
  if(cosalpha > 0.0f) 
    return Ka / M_PI; 
  return Vector3f(0.0f, 0.0f, 0.0f);
}

}