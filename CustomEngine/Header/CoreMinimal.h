#pragma once

#include <cmath>

struct FVector
{
    float X, Y, Z;

    FVector() : X(0), Y(0), Z(0) {}
    FVector(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}

    FVector operator+(const FVector& Other) const { return FVector(X + Other.X, Y + Other.Y, Z + Other.Z); }
    FVector operator-(const FVector& Other) const { return FVector(X - Other.X, Y - Other.Y, Z - Other.Z); }
    FVector operator*(float Scale) const { return FVector(X * Scale, Y * Scale, Z * Scale); }
};

struct FRotator
{
    float Pitch, Yaw, Roll;

    FRotator() : Pitch(0), Yaw(0), Roll(0) {}
    FRotator(float InPitch, float InYaw, float InRoll) : Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}
};

struct FTransform
{
    FVector Location;
    FRotator Rotation;
    FVector Scale3D;

    FTransform() : Location(0, 0, 0), Rotation(0, 0, 0), Scale3D(1, 1, 1) {}
    FTransform(const FVector& InLocation, const FRotator& InRotation, const FVector& InScale)
        : Location(InLocation), Rotation(InRotation), Scale3D(InScale) {}
};

struct FMatrix
{
    float M[16];

    FMatrix()
    {
        for (int i = 0; i < 16; ++i) M[i] = 0.0f;
        M[0] = M[5] = M[10] = M[15] = 1.0f;
    }

    const float* GetPtr() const { return M; }

    FMatrix operator*(const FMatrix& Other) const
    {
        FMatrix Result;
        for (int c = 0; c < 4; ++c)
        {
            for (int r = 0; r < 4; ++r)
            {
                Result.M[c * 4 + r] = 
                    M[0 * 4 + r] * Other.M[c * 4 + 0] +
                    M[1 * 4 + r] * Other.M[c * 4 + 1] +
                    M[2 * 4 + r] * Other.M[c * 4 + 2] +
                    M[3 * 4 + r] * Other.M[c * 4 + 3];
            }
        }
        return Result;
    }
};

inline FMatrix MakeTranslationMatrix(const FVector& Loc)
{
    FMatrix Mat;
    Mat.M[12] = Loc.X;
    Mat.M[13] = Loc.Y;
    Mat.M[14] = Loc.Z;
    return Mat;
}

inline FMatrix MakeScaleMatrix(const FVector& Scale)
{
    FMatrix Mat;
    Mat.M[0] = Scale.X;
    Mat.M[5] = Scale.Y;
    Mat.M[10] = Scale.Z;
    return Mat;
}

inline FMatrix MakeRotationMatrix(const FRotator& Rot)
{
    FMatrix MatX, MatY, MatZ;
    float RadP = Rot.Pitch * 3.14159265f / 180.0f;
    float RadY = Rot.Yaw   * 3.14159265f / 180.0f;
    float RadR = Rot.Roll  * 3.14159265f / 180.0f;

    float cP = std::cos(RadP), sP = std::sin(RadP);
    MatX.M[5] = cP; MatX.M[6] = sP;
    MatX.M[9] = -sP; MatX.M[10] = cP;

    float cY = std::cos(RadY), sY = std::sin(RadY);
    MatY.M[0] = cY; MatY.M[2] = -sY;
    MatY.M[8] = sY; MatY.M[10] = cY;

    float cR = std::cos(RadR), sR = std::sin(RadR);
    MatZ.M[0] = cR; MatZ.M[1] = sR;
    MatZ.M[4] = -sR; MatZ.M[5] = cR;

    // Pitch * Yaw * Roll (combining rotations)
    return MatZ * MatY * MatX;
}

inline FMatrix MakeTransformMatrix(const FTransform& Transform)
{
    FMatrix S = MakeScaleMatrix(Transform.Scale3D);
    FMatrix R = MakeRotationMatrix(Transform.Rotation);
    FMatrix T = MakeTranslationMatrix(Transform.Location);
    // Model = Translate * Rotate * Scale
    return T * R * S;
}
