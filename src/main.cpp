#include <iostream>
#include <memory>
#include <Eigen/Dense>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "geometry/material.h"
#include "gp/gp_eval.h"
#include "io/config.h"
#include "rendering/renderer.h"

using namespace mupsi;

int main()
{
    std::cout << "mupsi v0.1 — mu + psi" << std::endl;

    // sanity check
    #ifdef _OPENMP
        printf("OpenMP Enabled. Version: %d\n", _OPENMP);

        int thread_count = 0;
        #pragma omp parallel reduction(+:thread_count)
        {
            thread_count = 1;
        }

        printf("Active thread: %d\n", thread_count);
    #else
        printf("OpenMP is not enabled. \n");
    #endif

    Config cfg;
    try {
        cfg = load_config("config.json");
        std::cout << "config loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "config: " << e.what() << " — using defaults" << std::endl;
    }

    g_rayTraceConfig = cfg.trace;
    g_gpSeed = cfg.seed;

    // GPScene scene(cfg.cell_size, cfg.length_scale, cfg.amplitude, cfg.points_per_cell);
    SDFScene scene; 

    for (auto& s : cfg.spheres) {
        auto mat = std::make_shared<Material>(s.material.Ka, s.material.has_emission, s.material.emission_value);
        scene.add(std::make_unique<SDFSphere>(s.center, s.radius, mat));
    }
    for (auto& c : cfg.cubes) {
        auto mat = std::make_shared<Material>(c.material.Ka, c.material.has_emission, c.material.emission_value);
        scene.add(std::make_unique<SDFCube>(c.center, c.size, mat));
    }

    for (auto& l : cfg.parallel_lights)
        scene.addParallelLight({l.direction.normalized(), l.intensity});

    Camera camera(cfg.cam_pos, cfg.cam_dir, cfg.cam_up, cfg.cam_fov, cfg.cam_aspect_ratio);

    SDFRenderer renderer(cfg.width, cfg.height);
    renderer.render(scene, camera);

    renderer.save("test.png");

    return 0;
}
