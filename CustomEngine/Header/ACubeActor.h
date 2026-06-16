#pragma once

#include "AActor.h"
#include "UStaticMeshComponent.h"
#include "URigidBodyComponent.h"

class ACubeActor : public AActor
{
public:
    ACubeActor()
    {

        RigidBody = CreateDefaultSubobject<URigidBodyComponent>("RigidBody");
        SetRootComponent(RigidBody.get());

        MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("CubeMesh");
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();

        if (MeshComponent)
            MeshComponent->SetupCubeMesh();

    }

    virtual void Tick(float DeltaTime) override
    {
        Super::Tick(DeltaTime);

    }

    URigidBodyComponent* GetRigidBodyComponent() const { return RigidBody.get(); }

protected:
    TSharedPtr<URigidBodyComponent>  RigidBody;
    TSharedPtr<UStaticMeshComponent> MeshComponent;

    using Super = AActor;
};
