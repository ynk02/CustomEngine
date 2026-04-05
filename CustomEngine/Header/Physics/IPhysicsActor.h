#pragma once

#include "CoreMinimal.h"
#include <string>
#include <memory>


// ============================================================
//  FPhysicsActorDesc
//  IPhysicsActor 생성 시 넘기는 초기화 데이터
// ============================================================
struct FPhysicsActorDesc
{
    FTransform  InitialTransform;   // 초기 위치/회전/스케일
    float       Mass        = 1.0f; // kg
    bool        bKinematic  = false;// true → 물리 시뮬레이션 제외 (Static/Kinematic)
    bool        bSimulate   = true; // false → 완전히 비활성화
    float       LinearDamping  = 0.05f;
    float       AngularDamping = 0.05f;
    // 충돌 박스 반경 (단순 AABB 충돌용)
    FVector     HalfExtents = FVector(0.5f);
};

// ============================================================
//  IPhysicsActor
//  물리 시뮬레이션의 개별 강체(Rigid Body) 추상 핸들.
//  RHI의 FRHIResource 포지션 — 구현체를 모른 채 인터페이스로만 사용.
// ============================================================
class IPhysicsActor
{
public:
    virtual ~IPhysicsActor() = default;

    // ---- Transform ----
    /** 현재 물리 시뮬레이션 결과 Transform 조회 */
    virtual FTransform  GetTransform()  const = 0;
    /** 외부(에디터 등)에서 강제로 위치를 세팅 (Teleport) */
    virtual void        SetTransform(const FTransform& InTransform) = 0;

    // ---- Dynamics ----
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

    // ---- AABB (충돌 데이터) ----
    virtual FVector     GetHalfExtents() const = 0;
};
