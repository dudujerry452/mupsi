#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <Eigen/Core>

using namespace Eigen;


namespace mupsi {

class CovarianceFunction {
public:
  virtual ~CovarianceFunction() = default;

  virtual float kappa(const Vector3f& p, const Vector3f& q) const = 0;
  virtual float getSigma() const = 0;

  virtual Vector3f getLengthScale() const = 0;
};


class SparseConvKernel : public CovarianceFunction {
public:
  ~SparseConvKernel() override = default;

  virtual float h(const Vector3f& s, const Vector3f& p) const = 0;

  // for condtioning
  virtual Vector3f h_grad(const Vector3f& C, const Vector3f& p) const = 0;
  // H_h · v  =  h(r) · [(-2/L²)·v + (4/L⁴)·(r·v)·r],  r = p - C
  virtual Vector3f h_hessian_vec(const Vector3f& C, const Vector3f& p, const Vector3f& v) const = 0;
  // g = (1/h''(0)) · delta, for 1/h''(0)
  virtual float oneOverSecondDerivative() const = 0;
  virtual float var(float impulseDensity) const = 0;

  virtual float getKernelRadius() const = 0;
};

class SparseSEKernel : public SparseConvKernel {
public:
  SparseSEKernel(float sigma, float kernelRadius, const Vector3f& lengthScale)
    : sigma_(sigma), kernelRadius_(kernelRadius), lengthScale_(lengthScale) {}

  float kappa(const Vector3f& p, const Vector3f& q) const override {
    return std::exp(-(p - q).squaredNorm() / (4.0f * lengthScale_.squaredNorm()));
  }
  float getSigma() const override { return sigma_; }
  Vector3f getLengthScale() const override { return lengthScale_; }

  // sparse conv part 

  float h(const Vector3f& s, const Vector3f& p) const override {
    return std::exp(-(p - s).squaredNorm() / lengthScale_.squaredNorm());
  }
  Vector3f h_grad(const Vector3f& C, const Vector3f& p) const override {
    float L2 = lengthScale_.squaredNorm();
    return h(C, p) * (-2.0f / L2) * (p - C);
  }
  // H_h · v = h(r) * ((-2/L²) * v + (4/L⁴) * (r·v) * r), r = p - C
  Vector3f h_hessian_vec(const Vector3f& C, const Vector3f& p, const Vector3f& v) const override {
    float L2 = lengthScale_.squaredNorm();
    Vector3f r = p - C;
    float hv = h(C, p);
    return hv * ((-2.0f / L2) * v + (4.0f / (L2 * L2)) * (r.dot(v)) * r);
  }
  float oneOverSecondDerivative() const override {
    return -lengthScale_.squaredNorm() / 2.0f;
  }
  float var(float impulseDensity) const override {
    return (impulseDensity / std::pow(kernelRadius_, 3)) * std::pow(M_PI, 1.5f);
  }
  float getKernelRadius() const override { return kernelRadius_; }

private:
  float sigma_;
  float kernelRadius_;
  Vector3f lengthScale_;
};


// deprecated 
class SEKernel {
public:
  SEKernel(float sigma, float kernelRadius, const Vector3f& lengthScale)
    : sigma(sigma), kernelRadius(kernelRadius), lengthScale(lengthScale) {}

  float h(const Vector3f& s, const Vector3f& p) const {
    return std::exp(-(p - s).squaredNorm() / (lengthScale.squaredNorm()));
  }

  float kappa(const Vector3f& p, const Vector3f& q) const {
    return std::exp(-(p - q).squaredNorm() / (4 * lengthScale.squaredNorm()));
  }

  float var(float impulseDensity) const {
    return (impulseDensity / std::pow(kernelRadius, 3)) * std::pow(M_PI, 1.5);
  }

  Vector3f h_grad(const Vector3f& C, const Vector3f& p) const {
    float L2 = lengthScale.squaredNorm();
    return h(C, p) * (-2.0f / L2) * (p - C);
  }

  float oneOverSecondDerivative() const {
    return -lengthScale.squaredNorm() / 2.0f;
  }

  float getSigma() const { return sigma; }
  float getKernelRadius() const { return kernelRadius; }
  Vector3f getLengthScale() const { return lengthScale; }

private:
  float sigma;
  float kernelRadius;
  Vector3f lengthScale;
};

} // namespace mupsi

#endif
