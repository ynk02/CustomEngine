#pragma once

#include "UPrimitiveComponent.h"
#include <glad/glad.h>
#include <iostream>

class UStaticMeshComponent : public UPrimitiveComponent
{
public:
    UStaticMeshComponent() : VAO(0), VBO(0), EBO(0) {}
    virtual ~UStaticMeshComponent()
    {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();
    }

    void SetupCubeMesh()
    {
        // Simple cube geometry setup
        float vertices[] = {
            // positions          // normals
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        };
        unsigned int indices[] = {
            0, 1, 2, 2, 3, 0, // front
            4, 5, 6, 6, 7, 4, // back
            0, 4, 7, 7, 3, 0, // left
            1, 5, 6, 6, 2, 1, // right
            3, 2, 6, 6, 7, 3, // top
            0, 1, 5, 5, 4, 0  // bottom
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        
        bIsMeshSetup = true;
    }

    virtual void Render() override
    {
        if (!bIsActive || !bIsMeshSetup) return;

        // Draw the mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    unsigned int VAO, VBO, EBO;
    bool bIsMeshSetup = false;
    
    using Super = UPrimitiveComponent;
};
