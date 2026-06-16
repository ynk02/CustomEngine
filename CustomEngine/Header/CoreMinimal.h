#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

using FString = std::string;
using FVector = glm::vec3;
using FRotator = glm::vec3;
using FMatrix = glm::mat4;

struct FTransform
{
    FVector Location;
    FRotator Rotation;
    FVector Scale3D;

    FTransform() : Location(0.0f), Rotation(0.0f), Scale3D(1.0f) {}

    FTransform(const FVector& InLocation, const FRotator& InRotation, const FVector& InScale)
        : Location(InLocation), Rotation(InRotation), Scale3D(InScale) {}
};

inline FMatrix MakeTransformMatrix(const FTransform& Transform)
{
    glm::mat4 Mat(1.0f);

    Mat = glm::translate(Mat, Transform.Location);

    Mat = glm::rotate(Mat, glm::radians(Transform.Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    Mat = glm::rotate(Mat, glm::radians(Transform.Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    Mat = glm::rotate(Mat, glm::radians(Transform.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    Mat = glm::scale(Mat, Transform.Scale3D);

    return Mat;
}

inline FMatrix MakeProjectionMatrix(float FOV, float AspectRatio, float NearPlane, float FarPlane)
{
    return glm::perspective(glm::radians(FOV), AspectRatio, NearPlane, FarPlane);
}
