#pragma once

#include "IPhysicsSystem.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cfloat>

class ClassicalPhysicsActor final : public IPhysicsActor
{
public:
    explicit ClassicalPhysicsActor(const FPhysicsActorDesc& Desc)
        : Transform(Desc.InitialTransform)
        , Mass(Desc.Mass)
        , bKinematic(Desc.bKinematic)
        , bSimulate(Desc.bSimulate)
        , LinearDamping(Desc.LinearDamping)
        , AngularDamping(Desc.AngularDamping)
        , HalfExtents(Desc.HalfExtents)
        , LinearVelocity(0.0f)
    {}

    FTransform  GetTransform()  const override { return Transform; }
    void        SetTransform(const FTransform& T) override
    {
        Transform = T;
        LinearVelocity = FVector(0.0f);
    }

    void  SetMass(float InMass)  override { Mass = (InMass > 0.0f) ? InMass : 0.001f; }
    float GetMass()        const override { return Mass; }

    void  SetKinematic(bool bInK)      override { bKinematic = bInK; }
    bool  IsKinematic()          const override { return bKinematic; }

    void  SetSimulatePhysics(bool bSim) override { bSimulate = bSim; }
    bool  IsSimulatingPhysics()   const override { return bSimulate; }

    void  SetLinearDamping(float D)  override { LinearDamping  = D; }
    void  SetAngularDamping(float D) override { AngularDamping = D; }

    void AddForce(const FVector& Force) override
    {
        Acceleration += Force / Mass;
    }

    void AddImpulse(const FVector& Impulse) override
    {
        LinearVelocity += Impulse / Mass;
    }

    FVector GetLinearVelocity()           const override { return LinearVelocity; }
    void    SetLinearVelocity(const FVector& V) override { LinearVelocity = V; }

    FVector GetHalfExtents() const override { return HalfExtents; }

    void Step(float Dt, const FVector& Gravity)
    {
        if (!bSimulate || bKinematic) return;

        Acceleration += Gravity;

        LinearVelocity += Acceleration * Dt;
        LinearVelocity *= (1.0f - LinearDamping * Dt);

        Transform.Location += LinearVelocity * Dt;

        Acceleration = FVector(0.0f);
    }

    void ResolveFloorCollision(float FloorY)
    {
        if (!bSimulate || bKinematic) return;
        float bottom = Transform.Location.y - HalfExtents.y;
        if (bottom < FloorY)
        {
            Transform.Location.y = FloorY + HalfExtents.y;
            LinearVelocity.y = -LinearVelocity.y * 0.3f;
            if (std::abs(LinearVelocity.y) < 0.05f) LinearVelocity.y = 0.0f;
        }
    }

private:
    FTransform  Transform;
    float       Mass;
    bool        bKinematic;
    bool        bSimulate;
    float       LinearDamping;
    float       AngularDamping;
    FVector     HalfExtents;
    FVector     LinearVelocity;
    FVector     Acceleration = FVector(0.0f);
};

class ClassicalPhysicsScene final : public IPhysicsScene
{
public:

    void Initialize(const FVector& Gravity = FVector(0.0f, -9.81f, 0.0f)) override
    {
        WorldGravity = Gravity;
    }

    IPhysicsActor* CreateActor(const FPhysicsActorDesc& Desc) override
    {
        auto Actor = std::make_unique<ClassicalPhysicsActor>(Desc);
        ClassicalPhysicsActor* RawPtr = Actor.get();
        Actors.push_back(std::move(Actor));
        return RawPtr;
    }

    void RemoveActor(IPhysicsActor* Actor) override
    {
        Actors.erase(
            std::remove_if(Actors.begin(), Actors.end(),
                [Actor](const std::unique_ptr<ClassicalPhysicsActor>& a) { return a.get() == Actor; }),
            Actors.end());
    }

    void Simulate(float DeltaTime) override
    {
        for (auto& Actor : Actors)
        {
            Actor->Step(DeltaTime, WorldGravity);
            Actor->ResolveFloorCollision(0.0f);
        }
    }

    FRaycastHit Raycast(const FRay& Ray, float MaxDistance) const override
    {
        FRaycastHit BestHit;
        float ClosestT = MaxDistance;

        for (const auto& Actor : Actors)
        {
            FTransform T = Actor->GetTransform();
            FVector HE   = Actor->GetHalfExtents() * T.Scale3D;
            FVector Mn   = T.Location - HE;
            FVector Mx   = T.Location + HE;

            float tMin = -FLT_MAX, tMax = FLT_MAX;
            bool bHit = true;

            for (int Axis = 0; Axis < 3; ++Axis)
            {
                if (std::abs(Ray.Direction[Axis]) < 1e-6f)
                {
                    if (Ray.Origin[Axis] < Mn[Axis] || Ray.Origin[Axis] > Mx[Axis])
                    { bHit = false; break; }
                }
                else
                {
                    float t1 = (Mn[Axis] - Ray.Origin[Axis]) / Ray.Direction[Axis];
                    float t2 = (Mx[Axis] - Ray.Origin[Axis]) / Ray.Direction[Axis];
                    if (t1 > t2) std::swap(t1, t2);
                    tMin = std::max(tMin, t1);
                    tMax = std::min(tMax, t2);
                    if (tMin > tMax) { bHit = false; break; }
                }
            }

            if (bHit)
            {
                float tHit = (tMin > 0.0f) ? tMin : tMax;
                if (tHit > 0.0f && tHit < ClosestT)
                {
                    ClosestT = tHit;
                    BestHit.bHit        = true;
                    BestHit.Distance    = tHit;
                    BestHit.HitLocation = Ray.Origin + Ray.Direction * tHit;
                    BestHit.HitActor    = Actor.get();
                }
            }
        }
        return BestHit;
    }

    void    SetGravity(const FVector& Gravity) override { WorldGravity = Gravity; }
    FVector GetGravity()                 const override { return WorldGravity; }

private:
    std::vector<std::unique_ptr<ClassicalPhysicsActor>> Actors;
    FVector WorldGravity = FVector(0.0f, -9.81f, 0.0f);
};

class ClassicalPhysicsSystem final : public IPhysicsSystem
{
public:
    bool Initialize() override { return true; }
    void Shutdown()   override {}

    std::unique_ptr<IPhysicsScene> CreateScene() override
    {
        return std::make_unique<ClassicalPhysicsScene>();
    }
};
