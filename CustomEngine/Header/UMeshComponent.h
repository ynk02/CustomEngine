#pragma once
#include <glad/glad.h>
#include "UActorComponent.h"

class UMeshComponent : public UActorComponent
{
public:
    virtual ~UMeshComponent() = default;

    void InitializeBoxMesh()
    {

    }

    void Render()
    {
        if (!bIsActive) return;

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
};
