#pragma once

#include <cmath>
#include <cstdint>

namespace sdk {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kRadToDeg = 180.0 / kPi;

struct FVector {
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;

    FVector operator+(const FVector& other) const { return {X + other.X, Y + other.Y, Z + other.Z}; }
    FVector operator-(const FVector& other) const { return {X - other.X, Y - other.Y, Z - other.Z}; }
    FVector operator*(double scalar) const { return {X * scalar, Y * scalar, Z * scalar}; }
    FVector operator/(double scalar) const { return {X / scalar, Y / scalar, Z / scalar}; }

    double length() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    double distance(const FVector& other) const { return (*this - other).length(); }
    bool empty() const { return X == 0.0 && Y == 0.0 && Z == 0.0; }
};

struct FRotator {
    double Pitch = 0.0;
    double Yaw = 0.0;
    double Roll = 0.0;
};

struct FVector2D {
    double X = 0.0;
    double Y = 0.0;
};

struct FQuat {
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
    double W = 0.0;
};

// Fortnite tourne en UE5 avec des coordonnees double precision :
// FVector = 24 octets, FTransform = 0x60 octets.
struct FTransform {
    FQuat Rotation;
    FVector Translation;
    FVector Scale3D;
};

struct FMatrix {
    double M[4][4] = {};
};

inline double normalize_axis(double angle) {
    while (angle > 180.0) {
        angle -= 360.0;
    }
    while (angle < -180.0) {
        angle += 360.0;
    }
    return angle;
}

inline void get_axes(const FRotator& rotation, FVector& forward, FVector& right, FVector& up) {
    const double pitch = rotation.Pitch * kDegToRad;
    const double yaw = rotation.Yaw * kDegToRad;
    const double roll = rotation.Roll * kDegToRad;

    const double sp = std::sin(pitch);
    const double cp = std::cos(pitch);
    const double sy = std::sin(yaw);
    const double cy = std::cos(yaw);
    const double sr = std::sin(roll);
    const double cr = std::cos(roll);

    forward = {cp * cy, cp * sy, sp};
    right = {cy * sp * sr - cr * sy, sy * sp * sr + cr * cy, -cp * sr};
    up = {-(cr * cy * sp + sr * sy), cy * sr - cr * sy * sp, cr * cp};
}

inline FMatrix to_matrix_with_scale(const FTransform& transform) {
    FMatrix matrix;

    matrix.M[3][0] = transform.Translation.X;
    matrix.M[3][1] = transform.Translation.Y;
    matrix.M[3][2] = transform.Translation.Z;
    matrix.M[3][3] = 1.0;

    const double x2 = transform.Rotation.X + transform.Rotation.X;
    const double y2 = transform.Rotation.Y + transform.Rotation.Y;
    const double z2 = transform.Rotation.Z + transform.Rotation.Z;

    const double xx2 = transform.Rotation.X * x2;
    const double yy2 = transform.Rotation.Y * y2;
    const double zz2 = transform.Rotation.Z * z2;
    const double yz2 = transform.Rotation.Y * z2;
    const double wx2 = transform.Rotation.W * x2;
    const double xy2 = transform.Rotation.X * y2;
    const double wz2 = transform.Rotation.W * z2;
    const double xz2 = transform.Rotation.X * z2;
    const double wy2 = transform.Rotation.W * y2;

    matrix.M[0][0] = (1.0 - (yy2 + zz2)) * transform.Scale3D.X;
    matrix.M[0][1] = (xy2 + wz2) * transform.Scale3D.X;
    matrix.M[0][2] = (xz2 - wy2) * transform.Scale3D.X;

    matrix.M[1][0] = (xy2 - wz2) * transform.Scale3D.Y;
    matrix.M[1][1] = (1.0 - (xx2 + zz2)) * transform.Scale3D.Y;
    matrix.M[1][2] = (yz2 + wx2) * transform.Scale3D.Y;

    matrix.M[2][0] = (xz2 + wy2) * transform.Scale3D.Z;
    matrix.M[2][1] = (yz2 - wx2) * transform.Scale3D.Z;
    matrix.M[2][2] = (1.0 - (xx2 + yy2)) * transform.Scale3D.Z;

    return matrix;
}

inline FMatrix multiply(const FMatrix& left, const FMatrix& right) {
    FMatrix result;
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            double sum = 0.0;
            for (int k = 0; k < 4; ++k) {
                sum += left.M[row][k] * right.M[k][column];
            }
            result.M[row][column] = sum;
        }
    }
    return result;
}

inline FRotator find_look_at_rotation(const FVector& source, const FVector& destination) {
    const FVector direction = destination - source;
    const double horizontal = std::sqrt(direction.X * direction.X + direction.Y * direction.Y);

    FRotator rotation;
    rotation.Pitch = std::atan2(direction.Z, horizontal) * kRadToDeg;
    rotation.Yaw = std::atan2(direction.Y, direction.X) * kRadToDeg;
    rotation.Roll = 0.0;
    return rotation;
}

inline FRotator shortest_delta(const FRotator& from, const FRotator& to) {
    FRotator delta;
    delta.Pitch = normalize_axis(to.Pitch - from.Pitch);
    delta.Yaw = normalize_axis(to.Yaw - from.Yaw);
    delta.Roll = normalize_axis(to.Roll - from.Roll);
    return delta;
}

inline bool world_to_screen(const FVector& world, const FVector& cameraLocation,
                            const FRotator& cameraRotation, double fov, double screenWidth,
                            double screenHeight, FVector2D& out) {
    const FVector delta = world - cameraLocation;

    FVector forward;
    FVector right;
    FVector up;
    get_axes(cameraRotation, forward, right, up);

    const FVector cameraSpace{
        delta.X * forward.X + delta.Y * forward.Y + delta.Z * forward.Z,
        delta.X * right.X + delta.Y * right.Y + delta.Z * right.Z,
        delta.X * up.X + delta.Y * up.Y + delta.Z * up.Z};

    if (cameraSpace.X <= 1.0) {
        return false;
    }

    const double halfFov = fov * 0.5 * kDegToRad;
    if (halfFov <= 0.0) {
        return false;
    }

    const double tanHalfHorizontal = std::tan(halfFov);
    const double aspect = screenHeight > 0.0 ? screenWidth / screenHeight : 1.0;
    const double tanHalfVertical = tanHalfHorizontal / (aspect > 0.0 ? aspect : 1.0);

    out.X = screenWidth * 0.5 + (cameraSpace.Y / cameraSpace.X) / tanHalfHorizontal * (screenWidth * 0.5);
    out.Y = screenHeight * 0.5 - (cameraSpace.Z / cameraSpace.X) / tanHalfVertical * (screenHeight * 0.5);

    return true;
}

inline double fov_to_pixels(double fovDegrees, double cameraFovDegrees, double screenWidth) {
    const double halfFov = fovDegrees * 0.5 * kDegToRad;
    const double halfCamera = cameraFovDegrees * 0.5 * kDegToRad;
    if (halfCamera <= 0.0) {
        return 0.0;
    }
    return std::tan(halfFov) / std::tan(halfCamera) * (screenWidth * 0.5);
}

} // namespace sdk
