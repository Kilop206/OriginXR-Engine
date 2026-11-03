#pragma once
#include "originxr/Math.hpp"
namespace originxr {
// API-neutral representations matching OpenXR's right-handed, Y-up, -Z forward space.
// The native OpenXR runtime will provide these values in a later milestone.
struct XRPose {
    Vec3 position{};
    float qx{0.f},qy{0.f},qz{0.f},qw{1.f};
};
struct XRFov {
    float angleLeft{0},angleRight{0},angleUp{0},angleDown{0}; // radians
};
Mat4 makeXRView(const XRPose& pose);
Mat4 makeXRProjection(const XRFov& fov,float nearZ=0.05f,float farZ=150.f);
} // namespace originxr
