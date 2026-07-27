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

    scene.addPrimitive(std::make_shared<Sphere>(Vector3f(0.0f, 0.0f, -500.0f), 100.0f, nullptr));

    std::shared_ptr<GPMedium> gpmedium = std::make_shared<GPMedium>(
        std::make_shared<SphereMeanFunction>(Vector3f(0.0f, 0.0f, -500.0f), 100.0f),
        std::make_shared<SparseGPNoiseGenerator>(
            std::make_shared<SparseSEKernel>(1.0f, 1.0f, Vector3f(1.0f, 1.0f, 1.0f)),
            3
        )
    );

    scene.setMedium(gpmedium);

    g_gpSettings.gpMode = GPSettings::GPCorrelationMode::SingleRealization; // Set GP mode to SingleRealization

    auto emission_texture = std::make_shared<ConstantTexture>(Vector3f(1.0f, 1.0f, 1.0f)); // White emission
    auto light = std::make_shared<Sphere>(Vector3f(200.0f, 0.0f, -300.0f), 100.0f, nullptr); 
    light->setEmission(emission_texture); 
    scene.addPrimitive(light);
    
    scene.setCamera(std::make_shared<Camera>(
        Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.0f, 1.0f, 0.0f),
        45.0f, 256, 256
    ));

    Renderer renderer;
    renderer.prepareRender(scene);
    renderer.startRender(scene, 1);
    renderer.afterRender();

    return 0;
}
