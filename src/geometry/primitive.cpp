#include "primitive.h"

namespace mupsi {

  std::shared_ptr<Bsdf> Primitive::default_bsdf_ = std::make_shared<LambertianBsdf>();
  
}