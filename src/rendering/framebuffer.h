#pragma once

#include <Eigen/Core>
#include <vector>
#include <string>

using Vec3f = Eigen::Vector3f;

namespace mupsi {

struct Color {
    Vec3f rgb;
};

class Framebuffer {
public:
    Framebuffer(int width, int height);

    void clear();

    Color&       operator()(int x, int y);
    const Color& operator()(int x, int y) const;

    // Progressive rendering: accumulate one more sample, store weighted average.
    void accumulate(int x, int y, const Vec3f& sample);

    // Number of samples accumulated so far (per pixel, all the same).
    int  sampleCount() const { return sampleCount_; }
    void incrementSampleCount() { sampleCount_++; }

    // Tone-mapped (Reinhard) LDR pixel for display, in [0,1] RGB.
    Vec3f tonemapped(int x, int y) const;

    void save(const std::string& filename) const;

    int width()  const { return w_; }
    int height() const { return h_; }

private:
    int w_, h_;
    int sampleCount_ = 0;
    std::vector<Color> pixels_;
    std::vector<Vec3f>  sum_;       // running sum for progressive averaging

    int idx(int x, int y) const { return y * w_ + x; }
};

} // namespace mupsi
