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

    Color&       operator()(int x, int y);
    const Color& operator()(int x, int y) const;

    void save(const std::string& filename) const;

    int width()  const { return w_; }
    int height() const { return h_; }

private:
    int w_, h_;
    std::vector<Color> pixels_;

    int idx(int x, int y) const { return y * w_ + x; }
};

} // namespace mupsi
