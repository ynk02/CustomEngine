#pragma once

#include "AActor.h"
#include "UStaticMeshComponent.h"
#include "URigidBodyComponent.h"    // 물리 컴포넌트 포함

// ============================================================
//  ACubeActor
//  UStaticMeshComponent (렌더링) + URigidBodyComponent (물리)
//  두 컴포넌트를 소유하는 기본 큐브 Actor.
//
//  Root: URigidBodyComponent (USceneComponent 상속)
//    └── UStaticMeshComponent (렌더링 전용)
//
//  물리 컴포넌트가 Root이므로 Simulate 결과가 바로
//  Actor::GetTransform()에 반영된다.
// ============================================================
class ACubeActor : public AActor
{
public:
    ACubeActor()
    {
        // ① 물리 컴포넌트를 Root로 설정
        RigidBody = CreateDefaultSubobject<URigidBodyComponent>("RigidBody");
        SetRootComponent(RigidBody.get());

        // ② 렌더링 컴포넌트는 일반 컴포넌트로 추가
        MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("CubeMesh");
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();

        if (MeshComponent)
            MeshComponent->SetupCubeMesh();

        // URigidBodyComponent의 물리 씬 등록은
        // UApplication::SpawnCube()에서 처리
        // (씬 포인터가 Actor 외부에 있으므로)
    }

    virtual void Tick(float DeltaTime) override
    {
        Super::Tick(DeltaTime);
        // RigidBody->TickComponent()가 물리 결과를 Transform에 동기화
        // (AActor::Tick이 모든 컴포넌트의 TickComponent를 호출)
    }

    URigidBodyComponent* GetRigidBodyComponent() const { return RigidBody.get(); }

protected:
    TSharedPtr<URigidBodyComponent>  RigidBody;
    TSharedPtr<UStaticMeshComponent> MeshComponent;

    using Super = AActor;
};
