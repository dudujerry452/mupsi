#ifndef _SCENE_H_
#define _SCENE_H_

#include "object.h"
#include "ray.h"
#include "gp/gpnoise.h"
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
    std::shared_ptr<Material> material;
  };

  struct ParallelLight {
    Vector3f direction;
    Vector3f intensity;
  };

  class SDFScene
  {
  public:
    SDFScene();
    virtual ~SDFScene() = default;

    void add(std::unique_ptr<SDFObject> obj);
    void addParallelLight(const ParallelLight& light) { parallel_lights.push_back(light); };

    float eval(const Vector3f &pos) const { const SDFObject* obj = nullptr; return eval(pos, obj); };
    virtual float eval(const Vector3f &pos, const SDFObject*& obj) const;
    Vector3f getNormal(const Vector3f &pos) const;

    protected:
    std::vector<std::unique_ptr<SDFObject>> objects;
    Bounding bounding;

    std::vector<ParallelLight> parallel_lights;

    public:
    friend Vector3f traceRay(const Ray &ray, SDFScene &scene, int depth);
  };

  // Thread-local per-bounce seed, written by traceRay, read by GPScene::eval.
  // Each OpenMP thread has its own copy — no data race on the shared GPScene instance.
  extern thread_local uint32_t g_cond_seed;

  class GPScene: public SDFScene
  {
  public:
    GPScene(const GPNoiseGenerator& gp): SDFScene(), gpnoise(gp) {};
    virtual ~GPScene() = default;

    float eval(const Vector3f &pos, const SDFObject*& obj) const override;

    void prepareConditioning(const Vector3f& C, const Vector3f& targetGrad, uint32_t nextSeed);
    std::pair<ConditioningState, uint32_t> activateNextConditioning();
    void restoreConditioning(const ConditioningState& cond, uint32_t seed);

    const GPNoiseGenerator& getGPNoise() const { return gpnoise; }

  private:
    GPNoiseGenerator gpnoise;

  };
}

#endif