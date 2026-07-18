#ifndef _SDF_H_
#define _SDF_H_

#include <memory>
#include "material.h"

#include <Eigen/Core>
using namespace Eigen;

namespace mupsi
{

  struct Bounding 
  {
    Vector3f minpos, maxpos; 
    Bounding(): minpos(Vector3f::Zero()), maxpos(Vector3f::Zero()) {};
    Bounding(const Vector3f& minpos, const Vector3f& maxpos): minpos(minpos), maxpos(maxpos) {};

    Vector3f getInterPos(const Vector3f& pos) const{
      return (pos - minpos); 
    };
    Vector3f getOuterPos(const Vector3f& pos) const{
      return (pos + minpos); 
    };

    Bounding operator*(const Bounding& other) const{
      return Bounding(minpos.cwiseMin(other.minpos), maxpos.cwiseMax(other.maxpos)); 
    }; // 求交
    Bounding operator+(const Bounding& other) const{
      return Bounding(minpos.cwiseMax(other.minpos), maxpos.cwiseMin(other.maxpos)); 
    }; // 求并
    
  };

  class SDFObject
  {
  public:
    SDFObject(std::shared_ptr<Material> material): material(material) {};
    virtual ~SDFObject() = default;

    virtual float eval(const Vector3f &point) const = 0;

    Bounding getBounding() const {return bounding; }
    std::shared_ptr<Material> getMaterial() const {return material; }

  protected: 
    Bounding bounding; 
    std::shared_ptr<Material> material;
  };

  class SDFSphere : public SDFObject
  {
  public:
    SDFSphere(const Vector3f &center, float radius, std::shared_ptr<Material> material);
    virtual ~SDFSphere() = default;

    float eval(const Vector3f &point) const override;

  private:
    Vector3f center;
    float radius;
  };

  class SDFCube : public SDFObject
  {
  public:
    SDFCube(const Vector3f &center, const Vector3f &size, std::shared_ptr<Material> material);
    virtual ~SDFCube() = default;

    float eval(const Vector3f &point) const override;

  private:
    Vector3f center;
    Vector3f halfSize;
  };
}

#endif