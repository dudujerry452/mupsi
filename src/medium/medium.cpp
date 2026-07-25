#include "medium.h"

namespace mupsi {

  std::shared_ptr<Bsdf> Medium::default_bsdf_ = std::make_shared<LambertianBsdf>();

}