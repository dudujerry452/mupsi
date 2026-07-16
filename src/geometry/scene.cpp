#include "scene.h"
#include <memory>
#include <iostream>
#include "../gp/gp_eval.h"

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

Intersection SDFScene::castRay(const Ray &ray) const
{
  float t = 0.0;
  bool hit = false;
  static const float eps = 0.01, dt = 0.1;
  static const int depth = 50, binarynum = 3;

  if (eval(ray.getOrigin()) < eps) // start point is inside the object
    ;                                               // not sure how to deal with it
  else
    for (int i = 0; i < depth; i++)
    {
      t += dt;
      Vector3f pos = ray.getOrigin() +
                     ray.getDirection() * t;
      float v = eval(pos);

      if (v < eps)
      {
        float l = -dt, r = 0, mid = -dt / 2;
        for (int j = 0; j < binarynum; j++)
        {
          float midv = eval(ray.getOrigin() + ray.getDirection() * (t + mid));
          if (midv < eps)
            r = mid;
          else
            l = mid;
          mid = (l+r)/2; 
        }
        hit = true;
        break;
      }
    }

  Vector3f normal = Vector3f::Zero();
  if (hit)
  {
    // calculate normal with difference
    Vector3f hitPos = ray.getOrigin() + ray.getDirection() * t;
    normal =
        {
            eval(hitPos + Vector3f{eps, 0, 0}) - eval(hitPos - Vector3f{eps, 0, 0}),
            eval(hitPos + Vector3f{0, eps, 0}) - eval(hitPos - Vector3f{0, eps, 0}),
            eval(hitPos + Vector3f{0, 0, eps}) - eval(hitPos - Vector3f{0, 0, eps})};
    normal.normalize();
  }

  return Intersection{hit, t, ray.getOrigin() + ray.getDirection() * t, normal}; // Placeholder normal, replace with actual normal calculation
}

float GPScene::eval(const Vector3f &pos) const
{
  float mu = SDFScene::eval(pos);

  SEKernel kernel(lengthScale_); 

  static float amplitude = 0.1;
  float psi = gp_eval(pos, kernel, 100, cellSize_, 42) * amplitude; 
  // std::cout << "mu = " << mu << ", psi = " << psi << std::endl;
  return mu + psi;
}


Intersection GPScene::castRay(const Ray &ray) const
{
  float t = 0.0;
  bool hit = false;
  static const float eps = 0.01, dt = 0.5;
  static const int depth = 20, binarynum = 3;

  if (eval(ray.getOrigin()) < eps) // start point is inside the object
    ;                                               // not sure how to deal with it
  else
    for (int i = 0; i < depth; i++)
    {
      t += dt;
      Vector3f pos = ray.getOrigin() +
                     ray.getDirection() * t;
      float v = eval(pos);

      if (v < eps)
      {
        float l = -dt, r = 0, mid = -dt / 2;
        for (int j = 0; j < binarynum; j++)
        {
          float midv = eval(ray.getOrigin() + ray.getDirection() * (t + mid));
          if (midv < eps)
            r = mid;
          else
            l = mid;
          mid = (l+r)/2; 
        }
        hit = true;
        break;
      }
    }

  Vector3f normal = Vector3f::Zero();
  if (hit)
  {
    // calculate normal with difference
    Vector3f hitPos = ray.getOrigin() + ray.getDirection() * t;
    normal =
        {
            eval(hitPos + Vector3f{eps, 0, 0}) - eval(hitPos - Vector3f{eps, 0, 0}),
            eval(hitPos + Vector3f{0, eps, 0}) - eval(hitPos - Vector3f{0, eps, 0}),
            eval(hitPos + Vector3f{0, 0, eps}) - eval(hitPos - Vector3f{0, 0, eps})};
    normal.normalize();
  }

  return Intersection{hit, t, ray.getOrigin() + ray.getDirection() * t, normal}; // Placeholder normal, replace with actual normal calculation
}