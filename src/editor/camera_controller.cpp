#include "camera_controller.h"
#include "rendering/camera.h"
#include <Eigen/Geometry>
#include <GLFW/glfw3.h>

namespace mupsi {

void CameraController::apply(GLFWwindow* window, float dt) {
    (void)dt;

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    static int lastState = GLFW_RELEASE;
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (state == GLFW_PRESS) {
        static double lastX = 0.0, lastY = 0.0;
        if (lastState == GLFW_RELEASE) { lastX = mx; lastY = my; }
        float dx = float(mx - lastX);
        float dy = float(my - lastY);
        yaw   += dx * lookSpeed;
        pitch -= dy * lookSpeed;
        pitch  = std::clamp(pitch, -1.4f, 1.4f);
        lastX = mx; lastY = my;
    }
    lastState = state;

    move_[0] = move_[1] = move_[2] = 0.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move_.z() += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move_.z() -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move_.x() -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move_.x() += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) move_.y() += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) move_.y() -= 1.0f;
    boosting_ = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
             || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

std::shared_ptr<Camera> CameraController::makeCamera(const Camera& current, float dt) const {
    Vector3f dir(std::cos(pitch) * std::sin(yaw),
                 std::sin(pitch),
                -std::cos(pitch) * std::cos(yaw));
    dir.normalize();
    Vector3f right = dir.cross(Vector3f(0.0f, 1.0f, 0.0f)).normalized();
    Vector3f up    = right.cross(dir).normalized();

    float speed = boosting_ ? fastSpeed : baseSpeed;
    Vector3f dpos = (dir * move_[2] + right * move_[0] + up * move_[1]) * speed * dt;

    return std::make_shared<Camera>(
        current.pos() + dpos, dir, Vector3f(0.0f, 1.0f, 0.0f),
        current.fov(), current.width(), current.height()
    );
}

void CameraController::syncFromCamera(const Camera& cam) {
    const Vector3f& d = cam.dir();
    yaw   = std::atan2(d.x(), -d.z());
    pitch = std::asin(std::clamp(d.y(), -1.0f, 1.0f));
}

} // namespace mupsi
