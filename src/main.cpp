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
    Eigen::Vector3d v(1.0, 2.0, 3.0);
    std::cout << "Eigen ok: " << v.transpose() << std::endl;

    cv::Mat img(64, 64, CV_8UC3, cv::Scalar(128, 0, 0));
    std::cout << "OpenCV ok: " << img.rows << "x" << img.cols << std::endl;

    SDFScene scene; 
    scene.add(std::make_unique<SDFSphere>(Vector3f{0.0, 0.0, 0.0}, 2.0)); 

    Camera camera(Vector3f{0.0, 0.0, -3.0}, Vector3f{0.0, 0.0, 1.0}, Vector3f{0.0, 1.0, 0.0}, 60.0f, 1.0f);

    Renderer renderer(256, 256);
    renderer.render(scene, camera);

    renderer.save("test.png");

    return 0;
}
