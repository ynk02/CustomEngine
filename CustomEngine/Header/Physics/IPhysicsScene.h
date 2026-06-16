#pragma once

#include "IPhysicsActor.h"
#include "FRay.h"
#include <memory>
#include <vector>
#include <string>

struct FRaycastHit
{
    bool            bHit        = false;
    float           Distance    = 0.0f;
    FVector         HitLocation = FVector(0.0f);
    IPhysicsActor*  HitActor    = nullptr;
};

class IPhysicsScene
{
public:
    virtual ~IPhysicsScene() = default;

    virtual void    Initialize(const FVector& Gravity = FVector(0.0f, -9.81f, 0.0f)) = 0;

    virtual IPhysicsActor*  CreateActor(const FPhysicsActorDesc& Desc) = 0;

    virtual void            RemoveActor(IPhysicsActor* Actor) = 0;

    virtual void    Simulate(float DeltaTime) = 0;

    virtual FRaycastHit Raycast(const FRay& Ray, float MaxDistance = 10000.0f) const = 0;

    virtual void    SetGravity(const FVector& Gravity) = 0;
    virtual FVector GetGravity() const = 0;
};
