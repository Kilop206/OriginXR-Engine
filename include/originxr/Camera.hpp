#pragma once
#include "originxr/Math.hpp"

namespace originxr {

struct Camera {
    Vec3 position{0.f, 1.65f, 4.f};
    float yaw{0.f};                // positive = turn right
    float pitch{0.f};              // positive = look up
    float verticalFovDegrees{70.f};
    float nearPlane{0.05f};
    float farPlane{150.f};
    float interPupillaryDistance{0.064f};
    float convergenceDistance{3.f};

    Vec3 forward() const;
    Vec3 right() const;
    Mat4 view(float eyeOffset=0.f) const;
    Mat4 projection(float aspectRatio, float eyeOffset=0.f) const;
    void reset();
};

} // namespace originxr
