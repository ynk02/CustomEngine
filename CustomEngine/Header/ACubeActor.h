#pragma once

#include "AActor.h"
#include "UStaticMeshComponent.h"

class ACubeActor : public AActor
{
public:
    ACubeActor()
    {
        // UE5 style: CreateDefaultSubobject in constructor
        MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("CubeMesh");
        
        // Set as Root Component
        SetRootComponent(MeshComponent.get());
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();

        // Initialize OpenGL buffers for the cube
        if (MeshComponent)
        {
            MeshComponent->SetupCubeMesh();
        }
    }
     
    virtual void Tick(float DeltaTime) override
    {
        Super::Tick(DeltaTime);

        // Example: Rotate the cube every frame
        if (auto Root = GetRootComponent())
        {
            FRotator CurrentRot = Root->GetRotation();
            CurrentRot.Yaw += 45.0f * DeltaTime; // Rotate 45 degrees per second
            Root->SetRotation(CurrentRot);
        }
    }

protected:
    TSharedPtr<UStaticMeshComponent> MeshComponent;
    
    using Super = AActor;
};
