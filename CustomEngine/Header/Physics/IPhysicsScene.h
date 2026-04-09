#pragma once

#include "IPhysicsActor.h"
#include "FRay.h"
#include <memory>
#include <vector>
#include <string>


struct FRaycastHit
{
    bool            bHit        = false;
    float           Distance    = 0.0f;         // 레이 원점에서의 거리
    FVector         HitLocation = FVector(0.0f);
    IPhysicsActor*  HitActor    = nullptr;      // 충돌한 물리 액터
};

class IPhysicsScene
{
public:
    virtual ~IPhysicsScene() = default;

    // ---- Lifecycle ----
    /** physicsScne 초기화 (중력 설정 등) */
    virtual void    Initialize(const FVector& Gravity = FVector(0.0f, -9.81f, 0.0f)) = 0;

    // ---- Actor Management ----

    virtual IPhysicsActor*  CreateActor(const FPhysicsActorDesc& Desc) = 0;

    /** 씬에서 제거하고 메모리 해제 */
    virtual void            RemoveActor(IPhysicsActor* Actor) = 0;

    // ---- Simulation ----

    virtual void    Simulate(float DeltaTime) = 0;

    virtual FRaycastHit Raycast(const FRay& Ray, float MaxDistance = 10000.0f) const = 0;

    // ---- World Settings ----
    virtual void    SetGravity(const FVector& Gravity) = 0;
    virtual FVector GetGravity() const = 0;
};
