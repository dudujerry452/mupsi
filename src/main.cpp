#include <iostream>
#include <memory>
#include <Eigen/Dense>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "rendering/renderer.h"

using namespace mupsi;

int main()
{
    std::cout << "mupsi v0.1 — μ + ψ" << std::endl;

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

    GPScene scene(3.0, 1.0f, 5.0f, 3, 42);  // cellSize, lengthScale, amplitude, pointsPerCell, seed (matching sparse-gpis single-realization defaults) 
    scene.add(std::make_unique<SDFSphere>(Vector3f{0.0, 0.0, 0.0}, 200.0)); 

    Camera camera(Vector3f{0.0, 0.0, -220}, Vector3f{0.0, 0.0, 1.0}, Vector3f{0.0, 1.0, 0.0}, 60.0f, 1.0f);
    // Camera camera(Vector3f{-5, 0.0, 0}, Vector3f{1.0, 0.0, 0.0}, Vector3f{0.0, 1.0, 0.0}, 60.0f, 1.0f);

    GPRenderer renderer(1024, 1024);
    renderer.render(scene, camera);

    renderer.save("test.png");

    return 0;
}
