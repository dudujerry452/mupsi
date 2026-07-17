#include "scene.h"
#include <memory>
#include <iostream>
#include "gp/gp_eval.h"

using namespace mupsi;

SDFScene::SDFScene()
{
}

void SDFScene::add(std::unique_ptr<SDFObject> obj)
{
  objects.push_back(std::move(obj));
  bounding = bounding + objects.back()->getBounding();
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

float GPScene::eval(const Vector3f &pos) const
{
  float mu = SDFScene::eval(pos);

  SEKernel kernel(lengthScale_); 

  float psi = gp_eval(pos, kernel, pointsPerCell_, cellSize_, seed_) * amplitude_; 
  // std::cout << "mu = " << mu << ", psi = " << psi << std::endl;
  return mu + psi;
}

