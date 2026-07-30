#include "framebuffer.h"
#include <algorithm>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

namespace mupsi
{

    Framebuffer::Framebuffer(int width, int height)
        : w_(width), h_(height), sampleCount_(0),
          pixels_(width * height), sum_(width * height)
    {
        clear();
    }

    void Framebuffer::clear() {
        sampleCount_ = 0;
        for (int i = 0; i < w_ * h_; i++) {
            pixels_[i].rgb = Vec3f::Zero();
            sum_[i] = Vec3f::Zero();
        }
    }

    Color &Framebuffer::operator()(int x, int y) { return pixels_[idx(x, y)]; }
    const Color &Framebuffer::operator()(int x, int y) const { return pixels_[idx(x, y)]; }

    void Framebuffer::accumulate(int x, int y, const Vec3f& sample) {
        int i = idx(x, y);
        sum_[i] += sample;
        // Running average: pixels_ = sum / sampleCount after finalize
        // For intermediate display, compute the average inline:
        float invN = 1.0f / (sampleCount_ + 1);
        pixels_[i].rgb = sum_[i] * invN;
    }

    Vec3f Framebuffer::tonemapped(int x, int y) const {
        const Vec3f& v = pixels_[idx(x, y)].rgb;
        // Reinhard tone mapping
        return Vec3f(
            v.x() / (1.0f + v.x()),
            v.y() / (1.0f + v.y()),
            v.z() / (1.0f + v.z())
        );
    }

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
                    img.at<cv::Vec3f>(h_ - 1 - y, x) = cv::Vec3f(c.z(), c.y(), c.x());
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
                    img.at<cv::Vec3b>(h_ - 1 - y, x) = cv::Vec3b(
                        uint8_t(std::clamp(tone(c.z()), 0.0f, 1.0f) * 255.0f),
                        uint8_t(std::clamp(tone(c.y()), 0.0f, 1.0f) * 255.0f),
                        uint8_t(std::clamp(tone(c.x()), 0.0f, 1.0f) * 255.0f));
                }
            cv::imwrite(filename, img);
        }
    }

} // namespace mupsi
