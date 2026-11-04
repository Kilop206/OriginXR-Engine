#include "originxr/editor/EditorModel.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
namespace originxr::editor {
void UndoHistory::checkpoint(const Scene& scene) {
    if(capacity_==0)return;
    if(undo_.size()>=capacity_)undo_.erase(undo_.begin());
    undo_.push_back(scene);
    redo_.clear();
}
bool UndoHistory::undo(Scene& scene) {
    if(undo_.empty())return false;
    redo_.push_back(scene);
    scene=std::move(undo_.back());undo_.pop_back();
    return true;
}
bool UndoHistory::redo(Scene& scene) {
    if(redo_.empty())return false;
    if(undo_.size()>=capacity_)undo_.erase(undo_.begin());
    undo_.push_back(scene);
    scene=std::move(redo_.back());redo_.pop_back();
    return true;
}
void UndoHistory::clear() { undo_.clear();redo_.clear(); }
Ray cameraRay(const Camera& camera,float x,float y,float w,float h) {
    if(w<=0 || h<=0) throw std::invalid_argument("Invalid picking viewport");
    const float aspect=w/h;
    const float halfHeight=std::tan(radians(camera.verticalFovDegrees)*0.5f);
    const float px=(2.f*x/w-1.f)*halfHeight*aspect;
    const float py=(1.f-2.f*y/h)*halfHeight;
    const Vec3 up=normalized(cross(camera.right(),camera.forward()));
    return {camera.position,normalized(camera.forward()+camera.right()*px+up*py)};
}
namespace {
// Inverse of a column-major affine matrix; allows ray tests against rotated, scaled meshes.
bool inverseAffine(const Mat4& m,Mat4& out) {
    const auto& a=m.v;
    const Vec3 col0{a[0],a[1],a[2]},col1{a[4],a[5],a[6]},col2{a[8],a[9],a[10]};
    const float det=dot(col0,cross(col1,col2));
    if(std::abs(det)<1e-9f)return false;
    const Vec3 r0=cross(col1,col2)*(1.f/det);
    const Vec3 r1=cross(col2,col0)*(1.f/det);
    const Vec3 r2=cross(col0,col1)*(1.f/det);
    out=Mat4::identity();
    out.v[0]=r0.x;out.v[4]=r0.y;out.v[8]=r0.z;
    out.v[1]=r1.x;out.v[5]=r1.y;out.v[9]=r1.z;
    out.v[2]=r2.x;out.v[6]=r2.y;out.v[10]=r2.z;
    const Vec3 t{a[12],a[13],a[14]};
    out.v[12]=-dot(r0,t);out.v[13]=-dot(r1,t);out.v[14]=-dot(r2,t);
    return true;
}
Vec3 transformDirection(const Mat4& m,const Vec3& v) {
    return {m.v[0]*v.x+m.v[4]*v.y+m.v[8]*v.z,
            m.v[1]*v.x+m.v[5]*v.y+m.v[9]*v.z,
            m.v[2]*v.x+m.v[6]*v.y+m.v[10]*v.z};
}
bool hitBox(const Vec3& origin,const Vec3& direction,const Vec3& half,float& t) {
    float lo=-std::numeric_limits<float>::infinity();
    float hi=std::numeric_limits<float>::infinity();
    for(int axis=0;axis<3;++axis) {
        const float o=axis==0?origin.x:(axis==1?origin.y:origin.z);
        const float d=axis==0?direction.x:(axis==1?direction.y:direction.z);
        const float bound=axis==0?half.x:(axis==1?half.y:half.z);
        if(std::abs(d)<1e-7f) {if(o< -bound || o>bound)return false;continue;}
        float a=(-bound-o)/d,b=(bound-o)/d;
        if(a>b)std::swap(a,b);
        lo=std::max(lo,a);hi=std::min(hi,b);
        if(hi<lo)return false;
    }
    t=lo>1e-5f?lo:hi;
    return t>1e-5f;
}
}
EntityId pickEntity(const Scene& scene,const Ray& ray) {
    float nearest=std::numeric_limits<float>::max();EntityId selected=0;
    for(const auto& e:scene.entities()) {
        if(!e.mesh.visible)continue;
        Mat4 inv;
        if(!inverseAffine(scene.worldMatrix(e.id),inv))continue;
        const Vec3 localOrigin=inv.transformPoint(ray.origin);
        const Vec3 localDirection=transformDirection(inv,ray.direction);
        const Vec3 half=e.mesh.asset=="builtin:plane"?Vec3{0.5f,0.01f,0.5f}:Vec3{0.5f,0.5f,0.5f};
        float distance=0.f;
        // The parameter t is unchanged by an affine inverse when direction is not renormalized.
        if(hitBox(localOrigin,localDirection,half,distance) && distance<nearest) {
            nearest=distance;selected=e.id;
        }
    }
    return selected;
}
} // namespace originxr::editor
