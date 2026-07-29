#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "controller/controller.h"
#include "rendering/framebuffer.h"
#include "rendering/renderer.h"

using namespace mupsi;

int main(int argc, char* argv[])
{
    std::cout << "mupsi v0.1 — mu + psi" << std::endl;

#ifdef _OPENMP
    printf("OpenMP Enabled. Version: %d\n", _OPENMP);
    int thread_count = 0;
    #pragma omp parallel reduction(+:thread_count)
    { thread_count = 1; }
    printf("Active thread: %d\n", thread_count);
#else
    printf("OpenMP is not enabled. \n");
#endif

    const char* configPath = (argc >= 2) ? argv[1] : "scene.json";
    const char* outputPath = (argc >= 3) ? argv[2] : "test.png";

    Controller ctrl;
    if (!ctrl.load(configPath)) {
        std::cerr << "Failed to load " << configPath << std::endl;
        return -1;
    }
    ctrl.setOutputPath(outputPath);

    ctrl.start();

    // Wait for frame
    while (!ctrl.isFrameReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cerr << "progress: " << ctrl.getRenderer()->getProgress() << "%" << std::endl; 
    }

    std::cout << "Render complete. Saving " << ctrl.outputPath() << "..." << std::endl;
    ctrl.getFrameBuffer().save(ctrl.outputPath());

    ctrl.stop();
    return 0;
}
