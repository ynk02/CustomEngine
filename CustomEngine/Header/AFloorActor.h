#pragma once

#include "AActor.h"
#include "UStaticMeshComponent.h"

class AFloorActor : public AActor
{
public:
    AFloorActor()
    {
        MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("FloorMesh");
        SetRootComponent(MeshComponent.get());
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        if (MeshComponent)
        {
            MeshComponent->SetupPlaneMesh(10.0f);
        }
    }

protected:
    TSharedPtr<UStaticMeshComponent> MeshComponent;
    using Super = AActor;
};
