#include "originxr/Math.hpp"
#include <stdexcept>

namespace originxr {
float dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}
Vec3 normalized(const Vec3& a) {
    const float length = std::sqrt(dot(a, a));
    return length > 1e-7f ? a * (1.f / length) : Vec3{};
}
Mat4 Mat4::identity() {
    Mat4 m;
    m.v[0] = m.v[5] = m.v[10] = m.v[15] = 1.f;
    return m;
}
Mat4 Mat4::translation(const Vec3& p) {
    Mat4 m = identity(); m.v[12] = p.x; m.v[13] = p.y; m.v[14] = p.z; return m;
}
Mat4 Mat4::scaling(const Vec3& s) {
    Mat4 m = identity(); m.v[0] = s.x; m.v[5] = s.y; m.v[10] = s.z; return m;
}
Mat4 Mat4::rotationX(float degrees) {
    Mat4 m = identity(); float c = std::cos(radians(degrees)), s = std::sin(radians(degrees));
    m.v[5] = c; m.v[6] = s; m.v[9] = -s; m.v[10] = c; return m;
}
Mat4 Mat4::rotationY(float degrees) {
    Mat4 m = identity(); float c = std::cos(radians(degrees)), s = std::sin(radians(degrees));
    m.v[0] = c; m.v[2] = -s; m.v[8] = s; m.v[10] = c; return m;
}
Mat4 Mat4::rotationZ(float degrees) {
    Mat4 m = identity(); float c = std::cos(radians(degrees)), s = std::sin(radians(degrees));
    m.v[0] = c; m.v[1] = s; m.v[4] = -s; m.v[5] = c; return m;
}
Mat4 Mat4::frustum(float l, float r, float b, float t, float n, float f) {
    if (!(n > 0.f && f > n && r > l && t > b)) throw std::invalid_argument("Invalid view frustum");
    Mat4 m;
    m.v[0] = 2.f*n/(r-l); m.v[5] = 2.f*n/(t-b);
    m.v[8] = (r+l)/(r-l); m.v[9] = (t+b)/(t-b);
    m.v[10] = -(f+n)/(f-n); m.v[11] = -1.f;
    m.v[14] = -2.f*f*n/(f-n);
    return m;
}
Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    const Vec3 f = normalized(target - eye);
    const Vec3 s = normalized(cross(f, up));
    const Vec3 u = cross(s, f);
    Mat4 m = identity();
    m.v[0]=s.x; m.v[4]=s.y; m.v[8]=s.z;  m.v[12]=-dot(s,eye);
    m.v[1]=u.x; m.v[5]=u.y; m.v[9]=u.z;  m.v[13]=-dot(u,eye);
    m.v[2]=-f.x;m.v[6]=-f.y;m.v[10]=-f.z;m.v[14]=dot(f,eye);
    return m;
}
Mat4 Mat4::operator*(const Mat4& b) const {
    Mat4 result;
    for (int col=0; col<4; ++col)
        for (int row=0; row<4; ++row)
            for (int k=0; k<4; ++k)
                result.v[col*4+row] += v[k*4+row]*b.v[col*4+k];
    return result;
}
Vec3 Mat4::transformPoint(const Vec3& p) const {
    const float x = v[0]*p.x+v[4]*p.y+v[8]*p.z+v[12];
    const float y = v[1]*p.x+v[5]*p.y+v[9]*p.z+v[13];
    const float z = v[2]*p.x+v[6]*p.y+v[10]*p.z+v[14];
    const float w = v[3]*p.x+v[7]*p.y+v[11]*p.z+v[15];
    if (std::abs(w) < 1e-7f) throw std::runtime_error("Point at infinity");
    return {x/w,y/w,z/w};
}
} // namespace originxr
