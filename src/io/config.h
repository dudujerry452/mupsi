#pragma once

#include <Eigen/Dense>

#include "rendering/trace.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mupsi {

struct Config {
    // trace
    RayTraceConfig trace;

    // scene
    float     cell_size       = 3.0f;
    float     length_scale    = 1.0f;
    float     amplitude       = 5.0f;
    int       points_per_cell = 3;
    uint32_t  seed            = 42;

    // objects
    struct Sphere {
        Eigen::Vector3f center;
        float radius;
    };
    std::vector<Sphere> spheres;

    // camera
    Eigen::Vector3f cam_pos{0.0f, 0.0f, -220.0f};
    Eigen::Vector3f cam_dir{0.0f, 0.0f, 1.0f};
    Eigen::Vector3f cam_up{0.0f, 1.0f, 0.0f};
    float cam_fov          = 60.0f;
    float cam_aspect_ratio = 1.0f;

    // canvas
    int width  = 256;
    int height = 256;
};

Config load_config(const std::string& path);

} // namespace mupsi
