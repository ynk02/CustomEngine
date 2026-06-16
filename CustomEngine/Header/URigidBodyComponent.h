#pragma once

#include "USceneComponent.h"
#include "Physics/IPhysicsActor.h"
#include "Physics/IPhysicsScene.h"

class URigidBodyComponent : public USceneComponent
{
public:
    URigidBodyComponent()
        : PhysicsScene(nullptr)
        , PhysicsActor(nullptr)
        , bSimulatePhysics(true)
        , Mass(1.0f)
        , LinearDamping(0.05f)
        , HalfExtents(0.5f)
    {}

    virtual ~URigidBodyComponent() override
    {

        if (PhysicsScene && PhysicsActor)
            PhysicsScene->RemoveActor(PhysicsActor);
    }

    void RegisterWithPhysicsScene(IPhysicsScene* Scene, const FTransform& InitialTrans)
    {
        PhysicsScene = Scene;

        FPhysicsActorDesc Desc;
        Desc.InitialTransform = InitialTrans;
        Desc.Mass             = Mass;
        Desc.bKinematic       = !bSimulatePhysics;
        Desc.bSimulate        = bSimulatePhysics;
        Desc.LinearDamping    = LinearDamping;
        Desc.HalfExtents      = HalfExtents;

        PhysicsActor = PhysicsScene->CreateActor(Desc);
    }

    virtual void BeginPlay() override
    {
        Super::BeginPlay();
    }

    virtual void TickComponent(float DeltaTime) override
    {
        Super::TickComponent(DeltaTime);

        if (!PhysicsActor || !PhysicsActor->IsSimulatingPhysics()) return;
        if (PhysicsActor->IsKinematic()) return;

        FTransform PhysResult = PhysicsActor->GetTransform();
        SetTransform(PhysResult);

    }

    void  SetSimulatePhysics(bool bSim)
    {
        bSimulatePhysics = bSim;
        if (PhysicsActor) PhysicsActor->SetSimulatePhysics(bSim);
    }
    bool  IsSimulatingPhysics() const { return bSimulatePhysics; }

    void  SetMass(float InMass)
    {
        Mass = InMass;
        if (PhysicsActor) PhysicsActor->SetMass(InMass);
    }
    float GetMass() const { return Mass; }

    void  SetLinearDampingValue(float D)
    {
        LinearDamping = D;
        if (PhysicsActor) PhysicsActor->SetLinearDamping(D);
    }
    float GetLinearDampingValue() const { return LinearDamping; }

    void  SetHalfExtents(const FVector& HE)
    {
        HalfExtents = HE;

    }

    void SyncTransformToPhysics(const FTransform& NewTransform)
    {
        SetTransform(NewTransform);
        if (PhysicsActor) PhysicsActor->SetTransform(NewTransform);
    }

    IPhysicsActor* GetPhysicsActor() const { return PhysicsActor; }

    void DetachFromPhysicsScene()
    {
        PhysicsScene = nullptr;
        PhysicsActor = nullptr;
    }

private:
    IPhysicsScene* PhysicsScene;
    IPhysicsActor* PhysicsActor;

    bool    bSimulatePhysics;
    float   Mass;
    float   LinearDamping;
    FVector HalfExtents;

    using Super = USceneComponent;
};
