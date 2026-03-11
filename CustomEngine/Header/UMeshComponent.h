#pragma once
#include "UActorComponent.h"

class UMeshComponent : public UActorComponent
{
public:
    virtual ~UMeshComponent() = default;

    // 박스의 정점(Vertex) 데이터를 OpenGL 버퍼(VAO, VBO)에 올리는 함수
    void InitializeBoxMesh()
    {

    }

    // 매 프레임 화면에 그리는 함수
    void Render()
    {
        if (!bIsActive) return;

        // OpenGL 렌더링 호출
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
};