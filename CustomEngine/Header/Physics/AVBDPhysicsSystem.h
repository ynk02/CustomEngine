#pragma once

#include "IPhysicsSystem.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cfloat>
#include <cmath>

// ============================================================
//  FAVBDConfig
//  에디터에서 실시간으로 조절 가능한 AVBD 솔버 파라미터
// ============================================================
struct FAVBDConfig
{
    int   SolverIterations  = 8;      // Gauss-Seidel 반복 횟수 (높을수록 안정, 느림)
    float Restitution       = 0.30f;  // 반발 계수 (0 = 완전비탄성, 1 = 완전탄성)
    float Friction          = 0.50f;  // Coulomb 마찰 계수
    float BaumgarteCoeff    = 0.20f;  // Baumgarte 안정화 계수 (관통 보정 강도)
    float PenetrationSlop   = 0.005f; // 허용 관통 깊이 (너무 작은 관통은 무시)
    float FloorY            = 0.0f;   // 정적 바닥 Y 좌표
};


// ============================================================
//  FContactConstraint
//  AVBD Velocity-Level Contact Constraint
//  두 물체(또는 바닥)의 접촉 정보를 기술하는 구조체.
//  Gauss-Seidel 반복 시 AccumulatedLambda 누적으로 clamping.
// ============================================================
struct FContactConstraint
{
    class AVBDPhysicsActor* BodyA        = nullptr;  // 항상 유효
    class AVBDPhysicsActor* BodyB        = nullptr;  // nullptr = 정적 바닥
    FVector                 Normal;                  // BodyA → BodyB 충돌 법선
    float                   Penetration  = 0.0f;     // 관통 깊이 (양수)
    float                   AccumLambda  = 0.0f;     // 누적 법선 충격량 (반복 clamping용)
};


// ============================================================
//  AVBDPhysicsActor — IPhysicsActor 구현체
//
//  Null 방식과의 핵심 차이:
//    Null  → 위치를 직접 튕겨서 충돌 해소
//    AVBD  → 속도(Velocity)에 Impulse를 적용해 충돌 해소
//            Semi-Implicit Euler: 속도 먼저 업데이트 → 위치 업데이트
// ============================================================
class AVBDPhysicsActor final : public IPhysicsActor
{
public:
    explicit AVBDPhysicsActor(const FPhysicsActorDesc& Desc)
        : Transform(Desc.InitialTransform)
        , Velocity(0.0f)
        , ExternalForce(0.0f)
        , Mass(glm::max(Desc.Mass, 0.001f))
        , InvMass(Desc.bKinematic ? 0.0f : (1.0f / glm::max(Desc.Mass, 0.001f)))
        , LinearDamping(Desc.LinearDamping)
        , HalfExtents(Desc.HalfExtents)
        , bKinematic(Desc.bKinematic)
        , bSimulate(Desc.bSimulate)
    {}

    // ---- IPhysicsActor Interface ----
    FTransform GetTransform()  const override { return Transform; }
    void       SetTransform(const FTransform& T) override
    {
        Transform = T;
        Velocity  = FVector(0.0f); // 텔레포트 시 속도 초기화
    }

    void  SetMass(float InMass) override
    {
        Mass    = glm::max(InMass, 0.001f);
        InvMass = bKinematic ? 0.0f : (1.0f / Mass);
    }
    float GetMass()        const override { return Mass; }

    void  SetKinematic(bool bK) override
    {
        bKinematic = bK;
        InvMass    = bK ? 0.0f : (1.0f / Mass);
    }
    bool  IsKinematic()          const override { return bKinematic; }

    void  SetSimulatePhysics(bool bSim) override { bSimulate = bSim; }
    bool  IsSimulatingPhysics()   const override { return bSimulate; }

    void  SetLinearDamping(float D)  override { LinearDamping = D; }
    void  SetAngularDamping(float D) override { /* 회전 미구현 */ }

    void AddForce(const FVector& Force) override
    {
        if (!bSimulate || bKinematic) return;
        ExternalForce += Force;
    }

    void AddImpulse(const FVector& Impulse) override
    {
        if (!bSimulate || bKinematic) return;
        Velocity += Impulse * InvMass;
    }

    FVector GetLinearVelocity()            const override { return Velocity; }
    void    SetLinearVelocity(const FVector& V) override { Velocity = V; }

    FVector GetHalfExtents() const override { return HalfExtents; }

    // ---- AVBD 내부 전용 (AVBDPhysicsScene에서 호출) ----

    float GetInvMass() const { return InvMass; }

    /**
     * Step 1: 외부 힘 + 중력 → 속도 업데이트 (Semi-Implicit Euler 1단계)
     * 이 시점에서는 위치를 아직 움직이지 않는다.
     */
    void IntegrateVelocity(float Dt, const FVector& Gravity)
    {
        if (!bSimulate || bKinematic) return;

        FVector Accel  = Gravity + (ExternalForce * InvMass);
        Velocity      += Accel * Dt;
        Velocity      *= glm::max(0.0f, 1.0f - LinearDamping * Dt);

        ExternalForce = FVector(0.0f);
    }

    /**
     * Step 3: 제약 해소 후 최종 속도로 위치 업데이트.
     * 위치 업데이트는 항상 속도 제약 해소(Constraint Solve) 이후에 수행.
     */
    void IntegratePosition(float Dt)
    {
        if (!bSimulate || bKinematic) return;
        Transform.Location += Velocity * Dt;
    }

private:
    FTransform Transform;
    FVector    Velocity;
    FVector    ExternalForce;
    float      Mass;
    float      InvMass;       // 0 = 움직이지 않음 (Kinematic / Static)
    float      LinearDamping;
    FVector    HalfExtents;
    bool       bKinematic;
    bool       bSimulate;

    friend class AVBDPhysicsScene; // Velocity에 직접 접근 허용
};


// ============================================================
//  AVBDPhysicsScene — IPhysicsScene 구현체
//
//  시뮬레이션 루프:
//    1. IntegrateVelocity (외부 힘 + 중력)
//    2. GenerateContacts  (바닥 + 물체 간 AABB)
//    3. SolveContacts     (Gauss-Seidel Velocity Solver × N번)
//    4. IntegratePosition (최종 위치 업데이트)
// ============================================================
class AVBDPhysicsScene final : public IPhysicsScene
{
public:
    FAVBDConfig Config; // 에디터에서 직접 수정 가능

    // ---- IPhysicsScene Interface ----
    void Initialize(const FVector& Gravity = FVector(0.0f, -9.81f, 0.0f)) override
    {
        WorldGravity = Gravity;
    }

    IPhysicsActor* CreateActor(const FPhysicsActorDesc& Desc) override
    {
        auto  Actor  = std::make_unique<AVBDPhysicsActor>(Desc);
        auto* RawPtr = Actor.get();
        Actors.push_back(std::move(Actor));
        return RawPtr;
    }

    void RemoveActor(IPhysicsActor* Actor) override
    {
        Actors.erase(
            std::remove_if(Actors.begin(), Actors.end(),
                [Actor](const std::unique_ptr<AVBDPhysicsActor>& A)
                { return A.get() == Actor; }),
            Actors.end());
    }

    // ============================================================
    //  AVBD Simulate — 4단계 메인 루프
    // ============================================================
    void Simulate(float DeltaTime) override
    {
        // ── Step 1: 속도 적분 (중력 + 외력) ──────────────────
        for (auto& Actor : Actors)
            Actor->IntegrateVelocity(DeltaTime, WorldGravity);

        // ── Step 2: 충돌 감지 → Contact 목록 생성 ────────────
        Contacts.clear();
        GenerateFloorContacts();
        GenerateBodyContacts();

        // ── Step 3: Gauss-Seidel Velocity Constraint Solver ──
        //    N번 반복할수록 제약 해소가 더 정확해짐
        for (int Iter = 0; Iter < Config.SolverIterations; ++Iter)
            for (auto& C : Contacts)
                SolveContact(C, DeltaTime);

        // ── Step 4: 최종 위치 업데이트 ───────────────────────
        for (auto& Actor : Actors)
            Actor->IntegratePosition(DeltaTime);
    }

    FRaycastHit Raycast(const FRay& Ray, float MaxDistance) const override
    {
        FRaycastHit BestHit;
        float ClosestT = MaxDistance;

        for (const auto& Actor : Actors)
        {
            FTransform T  = Actor->GetTransform();
            FVector    HE = Actor->GetHalfExtents() * T.Scale3D;
            FVector    Mn = T.Location - HE;
            FVector    Mx = T.Location + HE;

            float tMin = -FLT_MAX, tMax = FLT_MAX;
            bool  bHit = true;

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
                    ClosestT             = tHit;
                    BestHit.bHit        = true;
                    BestHit.Distance    = tHit;
                    BestHit.HitLocation = Ray.Origin + Ray.Direction * tHit;
                    BestHit.HitActor    = Actor.get();
                }
            }
        }
        return BestHit;
    }

    void    SetGravity(const FVector& G) override { WorldGravity = G; }
    FVector GetGravity()           const override { return WorldGravity; }

private:
    // ============================================================
    //  Step 2a: 바닥(Y = Config.FloorY) 충돌 Contact 생성
    // ============================================================
    void GenerateFloorContacts()
    {
        for (auto& Actor : Actors)
        {
            if (!Actor->IsSimulatingPhysics() || Actor->IsKinematic()) continue;

            FTransform T      = Actor->GetTransform();
            FVector    HE     = Actor->GetHalfExtents() * T.Scale3D;
            float      Bottom = T.Location.y - HE.y;

            if (Bottom < Config.FloorY)
            {
                FContactConstraint C;
                C.BodyA       = Actor.get();
                C.BodyB       = nullptr;                          // static floor
                C.Normal      = FVector(0.0f, 1.0f, 0.0f);       // 위쪽
                C.Penetration = Config.FloorY - Bottom;
                Contacts.push_back(C);
            }
        }
    }

    // ============================================================
    //  Step 2b: 물체 간 AABB 브로드/내로우 페이즈
    //  최소 관통 축(SAT 단순화)으로 Contact Normal 결정
    // ============================================================
    void GenerateBodyContacts()
    {
        for (int i = 0; i < (int)Actors.size(); ++i)
        {
            for (int j = i + 1; j < (int)Actors.size(); ++j)
            {
                AVBDPhysicsActor* A = Actors[i].get();
                AVBDPhysicsActor* B = Actors[j].get();

                // 둘 다 시뮬레이션 안 하거나 Kinematic이면 스킵
                if (A->GetInvMass() < 1e-10f && B->GetInvMass() < 1e-10f) continue;
                if (!A->IsSimulatingPhysics() && !B->IsSimulatingPhysics()) continue;

                FTransform TA = A->GetTransform();
                FTransform TB = B->GetTransform();
                FVector    HA = A->GetHalfExtents() * TA.Scale3D;
                FVector    HB = B->GetHalfExtents() * TB.Scale3D;
                FVector    D  = TB.Location - TA.Location;

                // 각 축별 겹침 계산
                float OvX = (HA.x + HB.x) - std::abs(D.x);
                float OvY = (HA.y + HB.y) - std::abs(D.y);
                float OvZ = (HA.z + HB.z) - std::abs(D.z);

                if (OvX <= 0.0f || OvY <= 0.0f || OvZ <= 0.0f)
                    continue; // 분리 축 존재 → 충돌 없음

                // 최소 관통 축 선택 (SAT 기반 Contact Normal)
                FContactConstraint C;
                C.BodyA = A;
                C.BodyB = B;

                if (OvX <= OvY && OvX <= OvZ)
                {
                    C.Penetration = OvX;
                    C.Normal      = (D.x > 0.0f)
                                    ? FVector( 1.0f, 0.0f, 0.0f)
                                    : FVector(-1.0f, 0.0f, 0.0f);
                }
                else if (OvY <= OvX && OvY <= OvZ)
                {
                    C.Penetration = OvY;
                    C.Normal      = (D.y > 0.0f)
                                    ? FVector(0.0f,  1.0f, 0.0f)
                                    : FVector(0.0f, -1.0f, 0.0f);
                }
                else
                {
                    C.Penetration = OvZ;
                    C.Normal      = (D.z > 0.0f)
                                    ? FVector(0.0f, 0.0f,  1.0f)
                                    : FVector(0.0f, 0.0f, -1.0f);
                }

                Contacts.push_back(C);
            }
        }
    }

    // ============================================================
    //  Step 3: Velocity-Level Constraint Solver (AVBD 핵심)
    //
    //  법선 충격량:
    //    Jv     = dot(velA - velB, n)       // 법선 방향 상대속도
    //    rhs    = -(1+e)*Jv + baumgarte     // 목표 속도 변화량
    //    lambda = rhs / (invMassA+invMassB) // 충격량 크기
    //    lambda = max(AccumLambda + lambda, 0) - AccumLambda  (clamping)
    //
    //  접선 충격량 (Coulomb 마찰):
    //    frictionLambda = clamp(-|vt|/M_eff, -mu*lambda, mu*lambda)
    // ============================================================
    void SolveContact(FContactConstraint& C, float Dt)
    {
        AVBDPhysicsActor* A = C.BodyA;
        AVBDPhysicsActor* B = C.BodyB;

        float InvMassA    = A->GetInvMass();
        float InvMassB    = (B != nullptr) ? B->GetInvMass() : 0.0f;
        float TotalInvMass = InvMassA + InvMassB;
        if (TotalInvMass < 1e-10f) return;

        FVector VelA = A->Velocity;
        FVector VelB = (B != nullptr) ? B->Velocity : FVector(0.0f);
        FVector RelVel = VelA - VelB;

        // ── 법선 충격량 ─────────────────────────────────────────
        float Jv = glm::dot(RelVel, C.Normal); // 음수 = 접근 중

        // Baumgarte 안정화: 관통 깊이만큼 이번 스텝에서 속도로 보정
        float Baumgarte = (Config.BaumgarteCoeff / Dt)
                        * glm::max(C.Penetration - Config.PenetrationSlop, 0.0f);

        float Lambda = (-(1.0f + Config.Restitution) * Jv + Baumgarte) / TotalInvMass;

        // 누적 충격량 clamping (당기는 방향 제거)
        float OldAccum = C.AccumLambda;
        C.AccumLambda  = glm::max(OldAccum + Lambda, 0.0f);
        float ActualLambda = C.AccumLambda - OldAccum;

        FVector NormalImpulse = ActualLambda * C.Normal;
        A->Velocity +=  NormalImpulse * InvMassA;
        if (B) B->Velocity -= NormalImpulse * InvMassB;

        // ── 접선 충격량 (Coulomb 마찰) ───────────────────────────
        FVector RelVelAfter    = A->Velocity - (B ? B->Velocity : FVector(0.0f));
        FVector Tangent        = RelVelAfter - glm::dot(RelVelAfter, C.Normal) * C.Normal;
        float   TangentSpeed   = glm::length(Tangent);

        if (TangentSpeed > 1e-6f)
        {
            FVector TangentDir     = Tangent / TangentSpeed;
            float   FrictionLambda = -TangentSpeed / TotalInvMass;
            float   MaxFriction    = Config.Friction * C.AccumLambda;

            FrictionLambda = glm::clamp(FrictionLambda, -MaxFriction, MaxFriction);

            FVector FrictionImpulse = FrictionLambda * TangentDir;
            A->Velocity +=  FrictionImpulse * InvMassA;
            if (B) B->Velocity -= FrictionImpulse * InvMassB;
        }
    }

    std::vector<std::unique_ptr<AVBDPhysicsActor>> Actors;
    std::vector<FContactConstraint>                Contacts;
    FVector                                        WorldGravity = FVector(0.0f, -9.81f, 0.0f);
};


// ============================================================
//  AVBDPhysicsSystem — IPhysicsSystem 구현체
// ============================================================
class AVBDPhysicsSystem final : public IPhysicsSystem
{
public:
    bool Initialize() override { return true; }
    void Shutdown()   override {}

    std::unique_ptr<IPhysicsScene> CreateScene() override
    {
        return std::make_unique<AVBDPhysicsScene>();
    }
};
