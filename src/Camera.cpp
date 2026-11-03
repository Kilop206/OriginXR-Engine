#include "originxr/Camera.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace originxr {
Vec3 Camera::forward() const {
    const float y = radians(yaw), p = radians(pitch);
    return normalized({std::sin(y)*std::cos(p),std::sin(p),-std::cos(y)*std::cos(p)});
}
Vec3 Camera::right() const { return normalized(cross(forward(), {0.f,1.f,0.f})); }
Mat4 Camera::view(float offset) const {
    const Vec3 eye = position + right()*offset;
    return Mat4::lookAt(eye, eye+forward(), {0.f,1.f,0.f});
}
Mat4 Camera::projection(float aspect, float offset) const {
    if (!(aspect>0.f && convergenceDistance>0.f)) throw std::invalid_argument("Invalid camera aspect/convergence");
    const float hh = nearPlane*std::tan(radians(verticalFovDegrees)*0.5f);
    const float hw = hh*aspect;
    const float shift = -offset*nearPlane/convergenceDistance;
    return Mat4::frustum(-hw+shift,hw+shift,-hh,hh,nearPlane,farPlane);
}
void Camera::reset() { position={0.f,1.65f,4.f};yaw=0.f;pitch=0.f; }
}
