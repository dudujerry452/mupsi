#include "skydrome.h"
#include <cmath> 

namespace mupsi
{

  std::shared_ptr<Texture> Skydrome::default_emission_ = std::make_shared<ConstantTexture>(Vector3f(1.0f, 1.0f, 1.0f));
  
  bool Skydrome::intersect(Ray& ray, IntersectionTemporary& data) const
  {
    auto dir = ray.direction();
    float u = (std::atan2(dir.z(), dir.x()) + M_PI) / (2.0f * M_PI);
    float v = (std::acos(std::clamp(-dir.y(), -1.0f, 1.0f))) / M_PI; 
    data.as<SkydromeIntersection>()->u = u;
    data.as<SkydromeIntersection>()->v = v;
    data.primitive = this;
    return true; 
  }

  void Skydrome::intersectInfo(const IntersectionTemporary& data, IntersectionInfo& info) const
  {
    auto* si = data.as<SkydromeIntersection>();
    info.primitive = this;
    info.uv = Vector2f(si->u, si->v);
  }

} // namespace mupsi
