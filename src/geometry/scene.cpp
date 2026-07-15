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