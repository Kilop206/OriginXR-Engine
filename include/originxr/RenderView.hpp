#pragma once
#include "originxr/Math.hpp"
namespace originxr {
// An API-neutral view descriptor. Future OpenXR backend will supply these matrices
// from headset-tracked poses and field-of-view values, never from desktop IPD.
struct RenderView {
    Mat4 viewProjection{Mat4::identity()};
    int x{0}, y{0}, width{0}, height{0};
};
struct RenderStats {
    unsigned drawCalls{0};
    unsigned triangles{0};
    unsigned instances{0};
    double cpuMilliseconds{0.0};
};
} // namespace originxr
