#include "renderer.h"
#include <algorithm>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace mupsi;

int Renderer::getidx(int x, int y) const
{
  return y * width + x;
}

Renderer::Renderer(int width, int height) : width(width), height(height)
{
  framebuffer = new Color[width * height];
  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j < width; j++)
    {
      framebuffer[getidx(j, i)].rgb = Vector3f(i / float(height) * 255, 0, 0);
    }
  }
}

void Renderer::save(const std::string &filename) const
{
  // 判断是否 HDR 格式（EXR, HDR）
  bool isHdr = filename.ends_with(".exr") || filename.ends_with(".hdr");

  if (isHdr)
  {
    // 浮点通道直接写，不转换
    cv::Mat img(height, width, CV_32FC3);
    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        const auto &c = framebuffer[getidx(x, y)].rgb;
        img.at<cv::Vec3f>(height - 1 - y, x) = cv::Vec3f(c.x(), c.y(), c.z()); // 转换坐标系
      }
    }
    cv::imwrite(filename, img);
  }
  else
  {
    // LDR 格式: 色调映射 + 转 8-bit
    cv::Mat img(height, width, CV_8UC3);
    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        const auto &c = framebuffer[getidx(x, y)].rgb;
        // Reinhard 色调映射: x/(1+x), 然后 clamp 到 [0,1]
        auto tone = [](float v)
        { return v / (1.0f + v); };
        img.at<cv::Vec3b>(height - 1 - y, x) = cv::Vec3b(
            uint8_t(std::clamp(tone(c.x()), 0.0f, 1.0f) * 255.0f),
            uint8_t(std::clamp(tone(c.y()), 0.0f, 1.0f) * 255.0f),
            uint8_t(std::clamp(tone(c.z()), 0.0f, 1.0f) * 255.0f));
      }
    }
    cv::imwrite(filename, img);
  }
}