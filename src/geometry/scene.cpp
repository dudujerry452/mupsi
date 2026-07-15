#include "scene.hpp"
#include <memory>

using namespace mupsi;

Scene::Scene()
{
}

void Scene::add(std::unique_ptr<Object> obj)
{
  objects.push_back(std::move(obj));
}

SDFScene::SDFScene() : Scene()
{
}

float SDFScene::eval(const Vector3f &pos) const
{
  float min_distance = std::numeric_limits<float>::max();
  for (const auto &obj : objects)
  {
    auto obj_ptr = dynamic_cast<const SDFObject *>(obj.get());
    if (obj_ptr)
      min_distance = std::min(obj_ptr->eval(pos), min_distance);
  }
  return min_distance;
}