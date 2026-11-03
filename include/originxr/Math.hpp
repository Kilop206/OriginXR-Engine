#pragma once
#include <array>
#include <cmath>

namespace originxr {

constexpr float kPi = 3.14159265358979323846f;
inline float radians(float degrees) { return degrees * kPi / 180.0f; }

struct Vec3 {
    float x{0.f}, y{0.f}, z{0.f};
    Vec3 operator+(const Vec3& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    Vec3 operator-(const Vec3& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    Vec3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    Vec3& operator+=(const Vec3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
};

float dot(const Vec3& a, const Vec3& b);
Vec3 cross(const Vec3& a, const Vec3& b);
Vec3 normalized(const Vec3& a);

// Column-major matrices, compatible with OpenGL's glUniformMatrix4fv.
struct Mat4 {
    std::array<float, 16> v{};
    static Mat4 identity();
    static Mat4 translation(const Vec3& p);
    static Mat4 scaling(const Vec3& s);
    static Mat4 rotationX(float degrees);
    static Mat4 rotationY(float degrees);
    static Mat4 rotationZ(float degrees);
    static Mat4 frustum(float left, float right, float bottom, float top,
                        float nearZ, float farZ);
    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
    Mat4 operator*(const Mat4& rhs) const;
    Vec3 transformPoint(const Vec3& p) const;
    const float* data() const { return v.data(); }
};

} // namespace originxr
