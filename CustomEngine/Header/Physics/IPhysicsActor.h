#pragma once

#include "CoreMinimal.h"
#include <string>
#include <memory>

struct FPhysicsActorDesc
{
    FTransform  InitialTransform;
    float       Mass        = 1.0f;
    bool        bKinematic  = false;
    bool        bSimulate   = true;
    float       LinearDamping  = 0.05f;
    float       AngularDamping = 0.05f;

    FVector     HalfExtents = FVector(0.5f);
};

class IPhysicsActor
{
public:
    virtual ~IPhysicsActor() = default;

    virtual FTransform  GetTransform()  const = 0;

    virtual void        SetTransform(const FTransform& InTransform) = 0;

    virtual void        SetMass(float InMass) = 0;
    virtual float       GetMass() const = 0;

    virtual void        SetKinematic(bool bInKinematic) = 0;
    virtual bool        IsKinematic() const = 0;

    virtual void        SetSimulatePhysics(bool bSimulate) = 0;
    virtual bool        IsSimulatingPhysics() const = 0;

    virtual void        SetLinearDamping(float Damping) = 0;
    virtual void        SetAngularDamping(float Damping) = 0;

    virtual void        AddForce(const FVector& Force) = 0;
    virtual void        AddImpulse(const FVector& Impulse) = 0;

    virtual FVector     GetLinearVelocity() const = 0;
    virtual void        SetLinearVelocity(const FVector& Vel) = 0;

    virtual FVector     GetHalfExtents() const = 0;
};
