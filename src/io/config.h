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

    // materials
    struct MaterialConfig {
        Eigen::Vector3f Ka             = Eigen::Vector3f(0.8f, 0.8f, 0.8f);
        bool            has_emission   = false;
        float           emission_value = 0.0f;
    };

    // objects
    struct Sphere {
        Eigen::Vector3f center;
        float           radius;
        MaterialConfig  material;
    };
    std::vector<Sphere> spheres;

    struct Cube {
        Eigen::Vector3f center;
        Eigen::Vector3f size;
        MaterialConfig  material;
    };
    std::vector<Cube> cubes;

    // camera
    Eigen::Vector3f cam_pos{0.0f, 0.0f, -220.0f};
    Eigen::Vector3f cam_dir{0.0f, 0.0f, 1.0f};
    Eigen::Vector3f cam_up{0.0f, 1.0f, 0.0f};
    float cam_fov          = 60.0f;
    float cam_aspect_ratio = 1.0f;

    // lights
    struct ParallelLightConfig {
        Eigen::Vector3f direction = Eigen::Vector3f(-1.0f, -1.0f, -0.5f);
        Eigen::Vector3f intensity = Eigen::Vector3f(1.0f, 1.0f, 1.0f);
    };
    std::vector<ParallelLightConfig> parallel_lights;

    // canvas
    int width  = 256;
    int height = 256;
};

Config load_config(const std::string& path);

} // namespace mupsi
