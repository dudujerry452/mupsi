#ifndef _MESH_H_ 
#define _MESH_H_ 

#include "geometry/primitive.h"
#include <Eigen/Core> 

using namespace Eigen;


namespace mupsi {


class  Mesh: public Primitive {



public:

  Mesh() = default;
  ~Mesh() = default;

  bool fetchFrom(const std::string& filename); 

  bool intersect(Ray& ray, IntersectionTemporary& data) const; 
  void intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const;
  bool occluded(const Ray& ray)const; 

  bool sampleDirect(const Vector3f& p, Sampler& sampler, LightSample& sample) const; 
  void prepareForRender();

  int bsdfNum() const; 
  const Bsdf* getBsdf(int index) const;  
  const Texture* getEmission() const; 
  
  void setEmission(std::shared_ptr<Texture> emission);
}; 


}

#endif 