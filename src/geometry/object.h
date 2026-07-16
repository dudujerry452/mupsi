#ifndef _SDF_H_
#define _SDF_H_

#include <eigen3/Eigen/Eigen>
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


  class Object
  {
  public:
    Object() = default;
    virtual ~Object() = default;

    virtual Bounding getBounding() const = 0;
  
  protected: 
    Bounding bounding; 
  };

  class SDFObject : public Object
  {
  public:
    SDFObject() = default;
    virtual ~SDFObject() = default;

    virtual float eval(const Vector3f &point) const = 0;
  };

  class SDFSphere : public SDFObject
  {
  public:
    SDFSphere(const Vector3f &center, float radius);
    virtual ~SDFSphere() = default;

    float eval(const Vector3f &point) const override;

  private:
    Vector3f center;
    float radius;
  };
}

#endif