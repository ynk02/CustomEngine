#pragma once

#include "AActor.h"
#include "UStaticMeshComponent.h"

class ACubeActor : public AActor
{
public:
    ACubeActor()
    {
        MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("CubeMesh");
        
        // Set as Root Component
        SetRootComponent(MeshComponent.get());
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();

        if (MeshComponent)
        {
            MeshComponent->SetupCubeMesh();
        }
    }
    
    virtual void Tick(float DeltaTime) override
    {
        Super::Tick(DeltaTime);

        if (auto Root = GetRootComponent())
        {
            FRotator CurrentRot = Root->GetRotation();
            CurrentRot.y += 45.0f * DeltaTime; 
            Root->SetRotation(CurrentRot);
        }
    }

protected:
    TSharedPtr<UStaticMeshComponent> MeshComponent;
    
    using Super = AActor;
};
