#include "texture.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

namespace mupsi {

BitmapTexture::BitmapTexture(const std::string& filename) {
  load(filename);
}

bool BitmapTexture::load(const std::string& filename) {
  // HDR/EXR: load as float; LDR (PNG/JPG): load as 8-bit
  std::string ext = filename.substr(filename.find_last_of('.'));
  bool isHdr = (ext == ".hdr" || ext == ".exr");

  int flags = isHdr ? (cv::IMREAD_ANYDEPTH | cv::IMREAD_COLOR) : cv::IMREAD_COLOR;
  cv::Mat img = cv::imread(filename, flags);
  if (img.empty()) {
    std::cerr << "BitmapTexture: cannot load " << filename << std::endl;
    return false;
  }
  // OpenCV loads as BGR — convert to RGB
  cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

  w_ = img.cols;
  h_ = img.rows;
  data_.resize(w_ * h_);

  if (isHdr) {
    for (int y = 0; y < h_; ++y) {
      const float* row = img.ptr<float>(y);
      for (int x = 0; x < w_; ++x) {
        data_[y * w_ + x] = Vector3f(row[3*x], row[3*x+1], row[3*x+2]);
      }
    }
  } else {
    for (int y = 0; y < h_; ++y) {
      const uint8_t* row = img.ptr<uint8_t>(y);
      for (int x = 0; x < w_; ++x) {
        Vector3f c(row[3*x] / 255.0f, row[3*x+1] / 255.0f, row[3*x+2] / 255.0f);
        data_[y * w_ + x] = c;
      }
    }
  }

  std::cout << "BitmapTexture: loaded " << filename
            << " (" << w_ << "x" << h_ << ")" << std::endl;
  return true;
}

Vector3f BitmapTexture::operator[](const Vector2f& uv) const {
  if (data_.empty()) return Vector3f(1.0f, 0.0f, 1.0f); // magenta = error

  // Wrap to [0, 1]
  float u = uv.x() - std::floor(uv.x());
  float v = uv.y() - std::floor(uv.y());

  // OBJ UV: V=0 at bottom, V=1 at top.
  // Image: row 0 at top, row h-1 at bottom. Flip V.
  v = 1.0f - v;

  int x = static_cast<int>(u * w_) % w_;
  int y = static_cast<int>(v * h_) % h_;
  if (x < 0) x += w_;
  if (y < 0) y += h_;

  return data_[y * w_ + x];
}

}
