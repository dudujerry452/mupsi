#include <iostream>
#include <Eigen/Dense>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

int main() {
    std::cout << "mupsi v0.1 — μ + ψ" << std::endl;

    // sanity check
    Eigen::Vector3d v(1.0, 2.0, 3.0);
    std::cout << "Eigen ok: " << v.transpose() << std::endl;

    cv::Mat img(64, 64, CV_8UC3, cv::Scalar(128, 0, 0));
    std::cout << "OpenCV ok: " << img.rows << "x" << img.cols << std::endl;

    return 0;
}
