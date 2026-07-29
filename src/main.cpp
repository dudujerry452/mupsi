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
#include "bsdf/bsdf.h"

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

    std::shared_ptr<GPMedium> gpmedium = std::make_shared<GPMedium>(
        std::make_shared<SphereMeanFunction>(Vector3f(0.0f, 0.0f, -500.0f), 70.0f),
        std::make_shared<SparseGPNoiseGenerator>(
            std::make_shared<SparseSEKernel>(1.0f, 1.0f, Vector3f(1.0f, 1.0f, 1.0f)),
            3
        )
    );

    g_gpSettings.gpMode = GPSettings::GPCorrelationMode::SingleRealization; // Set GP mode to SingleRealization

    auto spotTex = std::make_shared<BitmapTexture>(
        "/home/dudujerry/models/spot/spot_texture.png");
    auto emission_texture = std::make_shared<ConstantTexture>(Vector3f(2.0f, 2.0f, 2.0f));

    auto spotBsdf = std::make_shared<LambertianBsdf>(spotTex);
    auto nullbsdf = std::make_shared<NullBsdf>();
    auto specularbsdf = std::make_shared<SpecularBsdf>();

    auto mesh = std::make_shared<Mesh>(specularbsdf);
    if (!mesh->fetchFrom("/home/dudujerry/models/spot/spot_triangulated_good.obj")) {
        std::cerr << "Failed to load mesh." << std::endl;
        return -1;
    }
    mesh->setTransform(Affine3f(
        Translation3f(0.0f, -100.0f, -420.0f) * Scaling(110.0f)
    ).matrix());

    auto sphere = std::make_shared<Sphere>(Vector3f(0.0f, 0.0f, -500.0f), 130.0f, nullbsdf);
    auto sphere2 = std::make_shared<Sphere>(Vector3f(0.0f, 100.0, -200.0f), 50.0f, specularbsdf);

    auto light = std::make_shared<Sphere>(Vector3f(200.0f, 0.0f, -300.0f), 100.0f, nullptr);
    auto light2 = std::make_shared<Sphere>(Vector3f(0.0f,  200.0f, -50.0f), 100.0f, nullptr);

    auto skydrome = std::make_shared<Skydrome>(
        std::make_shared<BitmapTexture>("/home/dudujerry/models/envmap.hdr")
    );

    light->setEmission(emission_texture);
    light2->setEmission(emission_texture);

    // scene.addPrimitive(mesh);

    scene.addPrimitive(sphere);
    scene.addPrimitive(sphere2);

    scene.addPrimitive(light);
    scene.addPrimitive(light2);

    sphere->setMedium(gpmedium, nullptr);

    scene.setSkydrome(skydrome);

    scene.setCamera(std::make_shared<Camera>(
        Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 1.0f, 0.0f),
        45.0f, 1024, 1024
    ));

    PathTracer::settings().max_bounce = 3;

    Renderer renderer;
    renderer.prepareRender(scene);
    renderer.startRender(scene, 3);
    renderer.afterRender();

    return 0;
}
