#pragma once

#include <cmath>

namespace sdk {

struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vector3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Rotator {
    double pitch = 0.0;
    double yaw = 0.0;
    double roll = 0.0;
};

struct FQuat {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 1.0;
};

// UE5 : FTransform = FQuat (0x20) + FVector (0x18) + pad + FVector (0x18) + pad.
struct alignas(16) FTransform {
    FQuat rotation;
    Vector3 translation;
    double pad0 = 0.0;
    Vector3 scale{1.0, 1.0, 1.0};
    double pad1 = 0.0;
};

inline constexpr double kPi = 3.14159265358979323846;

inline double deg2rad(double degrees) {
    return degrees * kPi / 180.0;
}

inline double rad2deg(double radians) {
    return radians * 180.0 / kPi;
}

inline Vector3 subtract(const Vector3& a, const Vector3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vector3 add(const Vector3& a, const Vector3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline double dot(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline double length(const Vector3& value) {
    return std::sqrt(dot(value, value));
}

inline double distance(const Vector3& a, const Vector3& b) {
    return length(subtract(a, b));
}

inline Vector3 rotate(const FQuat& rotation, const Vector3& value) {
    const Vector3 axis{rotation.x, rotation.y, rotation.z};
    Vector3 twist{axis.y * value.z - axis.z * value.y,
                  axis.z * value.x - axis.x * value.z,
                  axis.x * value.y - axis.y * value.x};
    twist.x *= 2.0;
    twist.y *= 2.0;
    twist.z *= 2.0;

    return {value.x + rotation.w * twist.x + (axis.y * twist.z - axis.z * twist.y),
            value.y + rotation.w * twist.y + (axis.z * twist.x - axis.x * twist.z),
            value.z + rotation.w * twist.z + (axis.x * twist.y - axis.y * twist.x)};
}

inline Vector3 transform_position(const FTransform& transform, const Vector3& point) {
    return add(rotate(transform.rotation, point), transform.translation);
}

} // namespace sdk
