#pragma once

#include "UObject.h"

class AActor;

// UActorComponent Base Class
class UActorComponent : public UObject
{
public:
    UActorComponent() : bIsActive(true) {}
    virtual ~UActorComponent() = default;

    virtual void BeginPlay() {}
    virtual void TickComponent(float DeltaTime) {}

    AActor* GetOwner() const;

    bool IsActive() const { return bIsActive; }
    virtual void SetActive(bool bNewActive) { bIsActive = bNewActive; }

protected:
    bool bIsActive;
};