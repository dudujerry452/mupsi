#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <Eigen/Core>
#include <string>
#include <vector>

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

  Vector3f operator[](const Vector2f& uv) const override {
    (void)uv;
    return color_;
  }
};

// UV-mapped bitmap texture, loaded from image file via OpenCV.
class BitmapTexture : public Texture {
  int w_ = 0, h_ = 0;
  std::vector<Vector3f> data_; // row-major RGB

public:
  BitmapTexture() = default;
  BitmapTexture(const std::string& filename);
  virtual ~BitmapTexture() = default;

  bool load(const std::string& filename);

  // uv in [0,1]², repeat / clamp at edges.
  Vector3f operator[](const Vector2f& uv) const override;
};

}

#endif