#ifndef _EDITOR_CAMERA_CONTROLLER_H_
#define _EDITOR_CAMERA_CONTROLLER_H_

#include <Eigen/Core>
#include <memory>

struct GLFWwindow;
using namespace Eigen;

namespace mupsi {

class Camera;

// WASD + mouse look camera controller.
struct CameraController {
    float yaw   = 0.0f;
    float pitch = 0.0f;
    float baseSpeed = 50.0f;
    float fastSpeed = 200.0f;
    float lookSpeed = 0.002f;

    // Read input from window, update yaw/pitch/move state.
    void apply(GLFWwindow* window, float dt);

    // Build a new camera from current yaw/pitch/move relative to `current`.
    std::shared_ptr<Camera> makeCamera(const Camera& current, float dt) const;

    // Initialize yaw/pitch from existing camera direction.
    void syncFromCamera(const Camera& cam);

private:
    Vector3f move_{0.0f, 0.0f, 0.0f};
    bool boosting_ = false;
};

} // namespace mupsi

#endif
