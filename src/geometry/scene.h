#ifndef _SCENE_H_
#define _SCENE_H_

#include "object.h"
#include "ray.h"
#include <memory>
#include <vector>

namespace mupsi
{

  struct Intersection
  {
    bool hit;
    float t;
    Vector3f position;
    Vector3f normal;
  };

  class SDFScene
  {
  public:
    SDFScene();
    virtual ~SDFScene() = default;

    void add(std::unique_ptr<SDFObject> obj);

    virtual float eval(const Vector3f &pos) const;
    virtual Intersection castRay(const Ray &ray) const;

    protected:
    std::vector<std::unique_ptr<SDFObject>> objects;
    Bounding bounding;
  };

  class GPScene: public SDFScene
  {
  public:
    GPScene(float cellSize, float lengthScale, float amplitude, int pointsPerCell, uint32_t seed)
        : SDFScene(), cellSize_(cellSize), lengthScale_(lengthScale), amplitude_(amplitude), pointsPerCell_(pointsPerCell), seed_(seed) {};
    virtual ~GPScene() = default;

    float eval(const Vector3f &pos) const override;
    Intersection castRay(const Ray &ray) const override;

  private:
    float cellSize_;
    float lengthScale_;
    float amplitude_;
    int pointsPerCell_;
    uint32_t seed_;
  };
}

#endif