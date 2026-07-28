#include <iostream>
#include <memory>
#include <Eigen/Dense>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "primitives/sphere.h"
#include "texture/texture.h"
#include "gp/gpnoise.h"
#include "io/config.h"
#include "rendering/renderer.h"
#include "rendering/trace.h"
#include "medium/gpmedium.h"
#include "primitives/mesh.h"

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

    Scene scene;

    // scene.addPrimitive(std::make_shared<Sphere>(Vector3f(0.0f, 0.0f, -500.0f), 100.0f, nullptr));

    // std::shared_ptr<GPMedium> gpmedium = std::make_shared<GPMedium>(
    //     std::make_shared<SphereMeanFunction>(Vector3f(0.0f, 0.0f, -500.0f), 100.0f),
    //     std::make_shared<SparseGPNoiseGenerator>(
    //         std::make_shared<SparseSEKernel>(1.0f, 1.0f, Vector3f(1.0f, 1.0f, 1.0f)),
    //         3
    //     )
    // );

    // scene.setMedium(gpmedium);

    // g_gpSettings.gpMode = GPSettings::GPCorrelationMode::SingleRealization; // Set GP mode to SingleRealization

    // Bunny at ~original sphere scale: Scale(1111) makes 0.18-tall bunny → ~200 tall.
    // Center Y at 0 by translating down half height, nose at z=-445 (closest to camera).
    auto mesh = std::make_shared<Mesh>();
    if (!mesh->fetchFrom("/home/dudujerry/models/bunny/bunny.obj")) {
        std::cerr << "Failed to load mesh." << std::endl;
        return -1;
    }
    mesh->setTransform(Affine3f(
        Translation3f(0.0f, -100.0f, -420.0f) * Scaling(1111.0f)
    ).matrix());
    scene.addPrimitive(mesh);

    // auto sphere = std::make_shared<Sphere>(Vector3f(0.0f, -100.0f, -420.0f), 100.0f, nullptr);
    // scene.addPrimitive(sphere);

    auto emission_texture = std::make_shared<ConstantTexture>(Vector3f(10.0f, 10.0f, 10.0f));
    auto light = std::make_shared<Sphere>(Vector3f(200.0f, 0.0f, -300.0f), 100.0f, nullptr);
    light->setEmission(emission_texture);
    auto light2 = std::make_shared<Sphere>(Vector3f(0.0f,  200.0f, -50.0f), 100.0f, nullptr);
    light2->setEmission(emission_texture);
    scene.addPrimitive(light);
    scene.addPrimitive(light2);

    scene.setCamera(std::make_shared<Camera>(
        Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 1.0f, 0.0f),
        45.0f, 1024, 1024
    ));

    PathTracer::settings().max_bounce = 8;

    Renderer renderer;
    renderer.prepareRender(scene);
    renderer.startRender(scene, 10);
    renderer.afterRender();

    return 0;
}
