#ifndef _MEANFUNCTION_H_
#define _MEANFUNCTION_H_

#include <Eigen/Core>
#include "primitives/mesh.h"
using namespace Eigen;

namespace mupsi {
  class MeanFunction {

    public: 

    virtual ~MeanFunction() = default;
    virtual float eval(const Vector3f& p) const = 0; 
    virtual Vector3f gradient(const Vector3f& p) const = 0;
  }; 

  class SphereMeanFunction : public MeanFunction {
    Vector3f center_;
    float radius_;

    public: 

    SphereMeanFunction(const Vector3f& center, float radius) : center_(center), radius_(radius) {}
    float eval(const Vector3f& p) const override; 
    Vector3f gradient(const Vector3f& p) const override;
  };

  class MeshMeanFunction : public MeanFunction {
    std::shared_ptr<Mesh> mesh_;

    public: 

    MeshMeanFunction(std::shared_ptr<Mesh> mesh) : mesh_(mesh) {
      mesh_->prepareForRender();
    }

    float eval(const Vector3f& p) const override;
    Vector3f gradient(const Vector3f& p) const override;
  };



}

#endif 