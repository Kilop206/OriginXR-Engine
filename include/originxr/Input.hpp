#pragma once
#include "originxr/Camera.hpp"
#include <array>
#include <cstddef>
namespace originxr {
// Device-independent input snapshot. A desktop, gamepad or XR action backend
// can populate it without introducing GLFW into the engine core.
enum class InputAction : std::size_t {
    Forward, Backward, Left, Right, Up, Down, Sprint, Count
};
struct InputState {
    std::array<bool,static_cast<std::size_t>(InputAction::Count)> held{};
    bool down(InputAction action) const {return held[static_cast<std::size_t>(action)];}
    void set(InputAction action,bool value) {held[static_cast<std::size_t>(action)]=value;}
};
void moveFreeCamera(Camera& camera,const InputState& input,float deltaSeconds);
} // namespace originxr
