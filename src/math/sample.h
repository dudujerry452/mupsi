#ifndef _SAMPLE_H_
#define _SAMPLE_H_

#include <Eigen/Core> 

using namespace Eigen; 

namespace mupsi {

struct TangentFrame
{
    Vector3f normal, tangent, bitangent;

    TangentFrame() = default;

    TangentFrame(const Vector3f &n, const Vector3f &t, const Vector3f &b)
    : normal(n.normalized()), tangent(t.normalized()), bitangent(b.normalized())
    {
    }

    TangentFrame(const Vector3f &n)
    : normal(n.normalized())
    {
        // [Duff et al. 17] Building An Orthonormal Basis, Revisited. JCGT. 2017.
        float sign = copysignf(1.0f, normal.z());
        const float a = -1.0f/(sign + normal.z());
        const float b = normal.x()*normal.y()*a;
        tangent = Vector3f(1.0f + sign*normal.x()*normal.x()*a, sign*b, -sign*normal.x());
        bitangent = Vector3f(b, sign + normal.y()*normal.y()*a, -normal.y());
        tangent.normalize();
        bitangent.normalize();
    }

    Vector3f toLocal(const Vector3f &p) const
    {
        return Vector3f(
            tangent.dot(p),
            bitangent.dot(p),
            normal.dot(p)
        );
    }

    Vector3f toGlobal(const Vector3f &p) const
    {
        return tangent*p.x() + bitangent*p.y() + normal*p.z();
    }
};

class SampleWrap {
  public: 
    
  static inline Vector3f uniformSphericalCap(const Vector2f &xi, float cosThetaMax)
  {
      float phi = xi.x()*2*M_PI;
      float z = xi.y()*(1.0f - cosThetaMax) + cosThetaMax;
      float r = std::sqrt(std::max(1.0f - z*z, 0.0f));
      return Vector3f(
          std::cos(phi)*r,
          std::sin(phi)*r,
          z
      );
  }
  static inline float uniformSphericalCapPdf(float cosThetaMax)
  {
    return 1.0f/(2.0f*M_PI*(1.0f - cosThetaMax));
  }

  }; 

}

#endif 