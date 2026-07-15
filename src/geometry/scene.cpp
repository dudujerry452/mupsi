#include "scene.hpp"
#include <memory>

using namespace mupsi;

Scene::Scene()
{
}

void Scene::add(std::unique_ptr<SDFObject> obj)
{
  objects.push_back(std::move(obj));
}

float Scene::eval(const Vector3f &pos) const
{
  float min_distance = std::numeric_limits<float>::max();
  for (const auto &obj : objects)
  {
    min_distance = std::min(obj->eval(pos), min_distance);
  }
  return min_distance;
}