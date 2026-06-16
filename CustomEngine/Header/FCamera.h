#pragma once

#include "CoreMinimal.h"

struct FCamera
{
    FVector Position   = { 0.0f,  3.0f, 10.0f };
    float   Yaw        = -90.0f;
    float   Pitch      = -15.0f;
    float   MoveSpeed  =   5.0f;
    float   Sensitivity =  0.15f;

    FVector Front    = {  0.0f, 0.0f, -1.0f };
    FVector Right    = {  1.0f, 0.0f,  0.0f };
    FVector WorldUp  = {  0.0f, 1.0f,  0.0f };

    void UpdateVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw))   * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw))   * cos(glm::radians(Pitch));
        Front   = glm::normalize(front);
        Right   = glm::normalize(glm::cross(Front, WorldUp));
    }

    FMatrix GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, WorldUp);
    }
};
