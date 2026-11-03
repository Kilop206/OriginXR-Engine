#include "originxr/XRMath.hpp"
#include <cmath>
#include <stdexcept>
namespace originxr {
Mat4 makeXRView(const XRPose& pose) {
    const float len2=pose.qx*pose.qx+pose.qy*pose.qy+pose.qz*pose.qz+pose.qw*pose.qw;
    if(!(len2>1e-8f))throw std::invalid_argument("Invalid XR quaternion");
    const float scale=1.f/std::sqrt(len2);
    const float x=pose.qx*scale,y=pose.qy*scale,z=pose.qz*scale,w=pose.qw*scale;
    const float r00=1-2*(y*y+z*z),r01=2*(x*y-z*w),r02=2*(x*z+y*w);
    const float r10=2*(x*y+z*w),r11=1-2*(x*x+z*z),r12=2*(y*z-x*w);
    const float r20=2*(x*z-y*w),r21=2*(y*z+x*w),r22=1-2*(x*x+y*y);
    // View matrix = inverse(T*R) = transpose(R) * T(-position).
    Mat4 out=Mat4::identity();
    out.v[0]=r00;out.v[4]=r10;out.v[8]=r20;
    out.v[1]=r01;out.v[5]=r11;out.v[9]=r21;
    out.v[2]=r02;out.v[6]=r12;out.v[10]=r22;
    out.v[12]=-(r00*pose.position.x+r10*pose.position.y+r20*pose.position.z);
    out.v[13]=-(r01*pose.position.x+r11*pose.position.y+r21*pose.position.z);
    out.v[14]=-(r02*pose.position.x+r12*pose.position.y+r22*pose.position.z);
    return out;
}
Mat4 makeXRProjection(const XRFov& fov,float nearZ,float farZ) {
    return Mat4::frustum(std::tan(fov.angleLeft)*nearZ,
                         std::tan(fov.angleRight)*nearZ,
                         std::tan(fov.angleDown)*nearZ,
                         std::tan(fov.angleUp)*nearZ,nearZ,farZ);
}
} // namespace originxr
