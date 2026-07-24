#include "framebuffer.h"
#include <algorithm>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

namespace mupsi
{

    Framebuffer::Framebuffer(int width, int height)
        : w_(width), h_(height), pixels_(width * height)
    {
        for (int y = 0; y < h_; y++)
            for (int x = 0; x < w_; x++)
                (*this)(x, y).rgb = Vec3f::Zero();
    }

    Color &Framebuffer::operator()(int x, int y) { return pixels_[idx(x, y)]; }
    const Color &Framebuffer::operator()(int x, int y) const { return pixels_[idx(x, y)]; }

    void Framebuffer::save(const std::string &filename) const
    {
        bool isHdr = filename.ends_with(".exr") || filename.ends_with(".hdr");

        if (isHdr)
        {
            cv::Mat img(h_, w_, CV_32FC3);
            for (int y = 0; y < h_; y++)
                for (int x = 0; x < w_; x++)
                {
                    const auto &c = (*this)(x, y).rgb;
                    img.at<cv::Vec3f>(y, x) = cv::Vec3f(c.x(), c.y(), c.z());
                }
            cv::imwrite(filename, img);
        }
        else
        {
            cv::Mat img(h_, w_, CV_8UC3);
            for (int y = 0; y < h_; y++)
                for (int x = 0; x < w_; x++)
                {
                    const auto &c = (*this)(x, y).rgb;
                    auto tone = [](float v)
                    { return v / (1.0f + v); };
                    img.at<cv::Vec3b>(y, x) = cv::Vec3b(
                        uint8_t(std::clamp(tone(c.x()), 0.0f, 1.0f) * 255.0f),
                        uint8_t(std::clamp(tone(c.y()), 0.0f, 1.0f) * 255.0f),
                        uint8_t(std::clamp(tone(c.z()), 0.0f, 1.0f) * 255.0f));
                }
            cv::imwrite(filename, img);
        }
    }

} // namespace mupsi
