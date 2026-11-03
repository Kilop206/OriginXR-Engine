#include "originxr/Input.hpp"
#include <algorithm>
#include <cmath>
namespace originxr {
void moveFreeCamera(Camera& camera,const InputState& input,float dt) {
    if(!std::isfinite(dt) || dt<=0)return;
    dt=std::clamp(dt,0.f,0.05f);
    Vec3 forward=camera.forward();forward.y=0;forward=normalized(forward);
    Vec3 right=camera.right();right.y=0;right=normalized(right);
    Vec3 velocity{};
    if(input.down(InputAction::Forward))velocity+=forward;
    if(input.down(InputAction::Backward))velocity+=forward*-1.f;
    if(input.down(InputAction::Right))velocity+=right;
    if(input.down(InputAction::Left))velocity+=right*-1.f;
    if(input.down(InputAction::Up))velocity.y+=1.f;
    if(input.down(InputAction::Down))velocity.y-=1.f;
    const float speed=input.down(InputAction::Sprint)?7.f:3.f;
    camera.position+=normalized(velocity)*(speed*dt);
    camera.position.y=std::max(camera.position.y,.15f);
}
} // namespace originxr
