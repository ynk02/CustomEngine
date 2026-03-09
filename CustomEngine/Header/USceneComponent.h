#pragma once

#include "UActorComponent.h"
#include "CoreMinimal.h"
#include <vector>

class USceneComponent : public UActorComponent
{
public:
    USceneComponent() {}
    virtual ~USceneComponent() = default;

    FTransform GetTransform() const { return ComponentTransform; }
    void SetTransform(const FTransform& InTransform) { ComponentTransform = InTransform; }

    FVector GetLocation() const { return ComponentTransform.Location; }
    void SetLocation(const FVector& InLocation) { ComponentTransform.Location = InLocation; }

    FRotator GetRotation() const { return ComponentTransform.Rotation; }
    void SetRotation(const FRotator& InRotation) { ComponentTransform.Rotation = InRotation; }

    // Hierarchy functionality
    void SetupAttachment(USceneComponent* InParent)
    {
        AttachParent = InParent;
        if (InParent)
        {
            InParent->AttachChildren.push_back(this);
        }
    }

    USceneComponent* GetAttachParent() const { return AttachParent; }
    const std::vector<USceneComponent*>& GetAttachChildren() const { return AttachChildren; }

protected:
    FTransform ComponentTransform;
    USceneComponent* AttachParent = nullptr;
    std::vector<USceneComponent*> AttachChildren;
};
