#include "scene.h"
#include <memory>
#include <iostream>
#include "gp/gp_eval.h"
#include "rendering/trace.h"

using namespace mupsi;

SDFScene::SDFScene()
{
}

void SDFScene::add(std::unique_ptr<SDFObject> obj)
{
  objects.push_back(std::move(obj));
  bounding = bounding + objects.back()->getBounding();
}

float SDFScene::eval(const Vector3f &pos, const SDFObject*& obj) const
{
  float min_distance = std::numeric_limits<float>::max();
  for (const auto &obj_ptr : objects)
  {
    float eval_value = obj_ptr->eval(pos);
    if(eval_value < min_distance) {
      min_distance = eval_value;
      obj = obj_ptr.get();
    }
  }
  return min_distance;
}

Vector3f SDFScene::getNormal(const Vector3f &pos) const {

  float eps = g_rayTraceConfig.eps;  // small value for numerical differentiation
  Vector3f normal;
  normal.x() = eval(pos + Vector3f(eps, 0, 0)) - eval(pos - Vector3f(eps, 0, 0));
  normal.y() = eval(pos + Vector3f(0, eps, 0)) - eval(pos - Vector3f(0, eps, 0));
  normal.z() = eval(pos + Vector3f(0, 0, eps)) - eval(pos - Vector3f(0, 0, eps));
  return normal.normalized();
}

float GPScene::eval(const Vector3f &pos, const SDFObject*& obj) const
{
  float mu = SDFScene::eval(pos, obj);

  SEKernel kernel(lengthScale_); 

  float psi = gp_eval(pos, kernel, pointsPerCell_, cellSize_) * amplitude_;
  // std::cout << "mu = " << mu << ", psi = " << psi << std::endl;
  return mu + psi;
}

