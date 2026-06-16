#pragma once

#include "IPhysicsSystem.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cfloat>

struct FXPBDConfig
{
    int   SolverIterations = 5;
    float FloorY           = 0.0f;
    float Compliance       = 1e-4f;
    float Restitution      = 0.3f;
    float Friction         = 0.5f;
};

class XPBDPhysicsActor final : public IPhysicsActor
{
public:
    explicit XPBDPhysicsActor(const FPhysicsActorDesc& Desc)
        : Transform(Desc.InitialTransform)
        , Mass(glm::max(Desc.Mass, 0.001f))
        , InvMass(Desc.bKinematic ? 0.0f : 1.0f / glm::max(Desc.Mass, 0.001f))
        , bKinematic(Desc.bKinematic)
        , bSimulate(Desc.bSimulate)
        , LinearDamping(Desc.LinearDamping)
        , AngularDamping(Desc.AngularDamping)
        , HalfExtents(Desc.HalfExtents)
        , Velocity(0.0f)
        , ExternalForce(0.0f)
    {}

    FTransform  GetTransform()  const override { return Transform; }
    void        SetTransform(const FTransform& T) override
    {
        Transform = T;
        Velocity = FVector(0.0f);
    }

    void  SetMass(float InMass)  override
    {
        Mass = glm::max(InMass, 0.001f);
        InvMass = bKinematic ? 0.0f : 1.0f / Mass;
    }
    float GetMass()        const override { return Mass; }

    void  SetKinematic(bool bInK)      override
    {
        bKinematic = bInK;
        InvMass = bInK ? 0.0f : 1.0f / Mass;
    }
    bool  IsKinematic()          const override { return bKinematic; }

    void  SetSimulatePhysics(bool bSim) override { bSimulate = bSim; }
    bool  IsSimulatingPhysics()   const override { return bSimulate; }

    void  SetLinearDamping(float D)  override { LinearDamping  = D; }
    void  SetAngularDamping(float D) override { AngularDamping = D; }

    void AddForce(const FVector& Force) override
    {
        if (bSimulate && !bKinematic)
            ExternalForce += Force;
    }

    void AddImpulse(const FVector& Impulse) override
    {
        if (bSimulate && !bKinematic)
            Velocity += Impulse * InvMass;
    }

    FVector GetLinearVelocity()           const override { return Velocity; }
    void    SetLinearVelocity(const FVector& V) override { Velocity = V; }

    FVector GetHalfExtents() const override { return HalfExtents; }

    void Predict(float Dt, const FVector& Gravity)
    {
        if (!bSimulate || bKinematic)
        {
            PredictedPos = Transform.Location;
            return;
        }

        FVector Accel = Gravity + (ExternalForce * InvMass);
        Velocity += Accel * Dt;
        Velocity *= (1.0f - LinearDamping * Dt);

        PredictedPos = Transform.Location + Velocity * Dt;

        ExternalForce = FVector(0.0f);
    }

    void EnsureGroundConstraint(float FloorY, float Compliance, float Dt)
    {
        if (!bSimulate || bKinematic) return;

        float bottom = PredictedPos.y - HalfExtents.y;
        float C = bottom - FloorY;

        if (C < 0.0f)
        {

            float alpha = Compliance / (Dt * Dt);
            float w = InvMass;

            float dLambda = -C / (w + alpha);
            FVector Normal(0.0f, 1.0f, 0.0f);

            PredictedPos += Normal * (dLambda * w);
        }
    }

    void UpdateVelocityAndApply(float Dt)
    {
        if (!bSimulate || bKinematic) return;

        FVector PrevVelocity = Velocity;
        Velocity = (PredictedPos - Transform.Location) / Dt;
        Transform.Location = PredictedPos;
    }

    float GetInvMass() const { return InvMass; }

    FTransform  Transform;
    FVector     PredictedPos;

    float       Mass;
    float       InvMass;
    bool        bKinematic;
    bool        bSimulate;

    float       LinearDamping;
    float       AngularDamping;
    FVector     HalfExtents;

    FVector     Velocity;
    FVector     ExternalForce;
};

class XPBDPhysicsScene final : public IPhysicsScene
{
public:

    void Initialize(const FVector& Gravity = FVector(0.0f, -9.81f, 0.0f)) override
    {
        WorldGravity = Gravity;
    }

    IPhysicsActor* CreateActor(const FPhysicsActorDesc& Desc) override
    {
        auto Actor = std::make_unique<XPBDPhysicsActor>(Desc);
        XPBDPhysicsActor* RawPtr = Actor.get();
        Actors.push_back(std::move(Actor));
        return RawPtr;
    }

    void RemoveActor(IPhysicsActor* Actor) override
    {
        Actors.erase(
            std::remove_if(Actors.begin(), Actors.end(),
                [Actor](const std::unique_ptr<XPBDPhysicsActor>& a) { return a.get() == Actor; }),
            Actors.end());
    }

    void Simulate(float DeltaTime) override
    {
        if (DeltaTime <= 0.0f) return;

        for (auto& Actor : Actors)
        {
            Actor->Predict(DeltaTime, WorldGravity);
        }

        for (int i = 0; i < Config.SolverIterations; ++i)
        {

            for (auto& Actor : Actors)
            {
                Actor->EnsureGroundConstraint(Config.FloorY, Config.Compliance, DeltaTime);
            }

            SolveBodyCollisions(DeltaTime);
        }

        for (auto& Actor : Actors)
        {
            Actor->UpdateVelocityAndApply(DeltaTime);
        }
    }

    void SolveBodyCollisions(float Dt)
    {

        float alpha = Config.Compliance / (Dt * Dt);

        for (int i = 0; i < (int)Actors.size(); ++i)
        {
            for (int j = i + 1; j < (int)Actors.size(); ++j)
            {
                XPBDPhysicsActor* A = Actors[i].get();
                XPBDPhysicsActor* B = Actors[j].get();

                float wA = A->GetInvMass();
                float wB = B->GetInvMass();
                if (wA + wB < 1e-6f) continue;

                FVector diff = B->PredictedPos - A->PredictedPos;
                FVector overlap = (A->GetHalfExtents() + B->GetHalfExtents()) - glm::abs(diff);

                if (overlap.x > 0.0f && overlap.y > 0.0f && overlap.z > 0.0f)
                {

                    int axis = 0;
                    float minOverlap = overlap.x;
                    if (overlap.y < minOverlap) { minOverlap = overlap.y; axis = 1; }
                    if (overlap.z < minOverlap) { minOverlap = overlap.z; axis = 2; }

                    FVector normal(0.0f);
                    normal[axis] = (diff[axis] > 0.0f) ? 1.0f : -1.0f;

                    float C = minOverlap;
                    float dLambda = C / (wA + wB + alpha);

                    FVector P = normal * dLambda;
                    if (A->bSimulate && !A->bKinematic) A->PredictedPos -= P * wA;
                    if (B->bSimulate && !B->bKinematic) B->PredictedPos += P * wB;
                }
            }
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

    FXPBDConfig Config;

private:
    std::vector<std::unique_ptr<XPBDPhysicsActor>> Actors;
    FVector WorldGravity = FVector(0.0f, -9.81f, 0.0f);
};

class XPBDPhysicsSystem final : public IPhysicsSystem
{
public:
    bool Initialize() override { return true; }
    void Shutdown()   override {}

    std::unique_ptr<IPhysicsScene> CreateScene() override
    {
        return std::make_unique<XPBDPhysicsScene>();
    }
};
