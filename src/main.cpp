#include <iostream>
#include <memory>
#include <Eigen/Dense>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "primitives/sphere.h"
#include "gp/gpnoise.h"
#include "io/config.h"
#include "rendering/renderer.h"
#include "rendering/trace.h"

using namespace mupsi;
using namespace Eigen; 

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

    // Config cfg;
    // try {
    //     cfg = load_config("config.json");
    //     std::cout << "config loaded successfully" << std::endl;
    // } catch (const std::exception& e) {
    //     std::cerr << "config: " << e.what() << " — using defaults" << std::endl;
    // }

    // g_rayTraceConfig = cfg.trace;

    // SEKernel kernel(3, cfg.cell_size, cfg.length_scale * Vector3f(1.0f, 1.0f, 1.0f));
    // GPNoiseGenerator gpnoise(kernel, cfg.points_per_cell, cfg.seed);

    // // Select scene type: "sdf" / "single_realization" / "ensemble_renewal_plus"
    // std::unique_ptr<SDFScene> scene;
    // if (cfg.gp_mode == "sdf") {
    //     scene = std::make_unique<SDFScene>();
    //     std::cout << "mode: SDF (no GP)" << std::endl;
    // } else {
    //     if (cfg.gp_mode == "single_realization")
    //         g_gpMode = GPCorrelationMode::SingleRealization;
    //     else if (cfg.gp_mode == "ensemble_renewal_plus")
    //         g_gpMode = GPCorrelationMode::RenewalPlus;
    //     std::cout << "mode: GP " << cfg.gp_mode << std::endl;
    //     scene = std::make_unique<GPScene>(gpnoise);
    // }

    // for (auto& s : cfg.spheres) {
    //     auto mat = std::make_shared<Material>(s.material.Ka, s.material.has_emission, s.material.emission_value);
    //     scene->add(std::make_unique<SDFSphere>(s.center, s.radius, mat));
    // }
    // for (auto& c : cfg.cubes) {
    //     auto mat = std::make_shared<Material>(c.material.Ka, c.material.has_emission, c.material.emission_value);
    //     scene->add(std::make_unique<SDFCube>(c.center, c.size, mat));
    // }

    // for (auto& l : cfg.parallel_lights)
    //     scene->addParallelLight({l.direction.normalized(), l.intensity});

    // Camera camera(cfg.cam_pos, cfg.cam_dir, cfg.cam_up, cfg.cam_fov, cfg.cam_aspect_ratio);

    // SDFRenderer renderer(cfg.width, cfg.height);
    // renderer.render(*scene, camera);

    // renderer.save("test.png");

        
    Scene scene;

    scene.addPrimitive(std::make_shared<Sphere>(Vector3f(0.0f, 0.0f, -5.0f), 1.0f, nullptr));
    scene.setCamera(std::make_shared<Camera>(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 1.0f, 0.0f), 45.0f, 256, 256));

    Renderer renderer;
    renderer.prepareRender(scene);
    renderer.startRender(scene);
    renderer.afterRender();

    return 0;
}
