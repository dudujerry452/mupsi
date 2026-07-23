#include "primitive.h"
#include "texture/texture.h"

namespace mupsi {

  std::shared_ptr<Bsdf> Primitive::default_bsdf_ = std::make_shared<LambertianBsdf>();
  std::shared_ptr<Texture> Primitive::default_emission_ = std::make_shared<ConstantTexture>(Vector3f(1.0, 1.0, 1.0));
  
}