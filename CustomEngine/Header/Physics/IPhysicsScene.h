#pragma once

#include "IPhysicsActor.h"
#include "FRay.h"
#include <memory>
#include <vector>
#include <string>

// ============================================================
//  FRaycastHit
//  Raycast 결과. 언리얼의 FHitResult 경량 버전.
// ============================================================
struct FRaycastHit
{
    bool            bHit        = false;
    float           Distance    = 0.0f;         // 레이 원점에서의 거리
    FVector         HitLocation = FVector(0.0f);
    IPhysicsActor*  HitActor    = nullptr;      // 충돌한 물리 액터 (소유권 없음)
};

// ============================================================
//  IPhysicsScene
//  한 월드의 물리 시뮬레이션 컨텍스트.
//  언리얼의 FPhysScene / PxScene wrapper 포지션.
// ============================================================
class IPhysicsScene
{
public:
    virtual ~IPhysicsScene() = default;

    // ---- Lifecycle ----
    /** physicsScne 초기화 (중력 설정 등) */
    virtual void    Initialize(const FVector& Gravity = FVector(0.0f, -9.81f, 0.0f)) = 0;

    // ---- Actor Management ----
    /**
     * 새 물리 액터를 씬에 추가 & 핸들 반환.
     * 반환된 포인터의 생존 기간은 씬이 소유 (RemoveActor 호출 전까지 유효).
     */
    virtual IPhysicsActor*  CreateActor(const FPhysicsActorDesc& Desc) = 0;

    /** 씬에서 제거하고 메모리 해제 */
    virtual void            RemoveActor(IPhysicsActor* Actor) = 0;

    // ---- Simulation ----
    /**
     * Fixed-step 물리 시뮬레이션 한 스텝 진행.
     * @param DeltaTime  고정 타임스텝 (예: 1/60 s)
     */
    virtual void    Simulate(float DeltaTime) = 0;

    // ---- Query ----
    /**
     * 레이캐스트. 가장 가까운 충돌체 반환.
     * MaxDistance 내에 충돌이 없으면 bHit = false.
     */
    virtual FRaycastHit Raycast(const FRay& Ray, float MaxDistance = 10000.0f) const = 0;

    // ---- World Settings ----
    virtual void    SetGravity(const FVector& Gravity) = 0;
    virtual FVector GetGravity() const = 0;
};
