#ifndef _TEXTURE_H_ 
#define _TEXTURE_H_

#include <Eigen/Core> 

using namespace Eigen;

namespace mupsi {



class Texture {

public: 
  Texture() = default; 
  virtual ~Texture() = default; 

  virtual Vector3f operator[](const Vector2f& uv) const = 0;
}; 

class ConstantTexture : public Texture {

  Vector3f color_; 

  public: 
    ConstantTexture(const Vector3f& color) : color_(color) {} 
    virtual ~ConstantTexture() = default; 

    virtual Vector3f operator[](const Vector2f& uv) const override {
      return color_; 
    }

};

  

}

#endif 