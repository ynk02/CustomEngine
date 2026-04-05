#pragma once

#include "USceneComponent.h"
#include "Physics/IPhysicsActor.h"     // 인터페이스만 참조 (구현체 모름)
#include "Physics/IPhysicsScene.h"

// ============================================================
//  URigidBodyComponent
//  언리얼의 UPrimitiveComponent::BodyInstance 역할을 담당.
//  USceneComponent를 상속해 Actor Transform과 직접 연결.
//
//  ★ 핵심 디커플링 ★
//    - IPhysicsActor 인터페이스 포인터만 소유 (raw, 소유권 없음)
//    - 실제 구현체(NullPhysicsActor, JoltActor 등)를 전혀 모름
//    - 물리 씬이 Actor의 생존 기간을 관리
// ============================================================
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
        // 씬에서 물리 액터 제거
        if (PhysicsScene && PhysicsActor)
            PhysicsScene->RemoveActor(PhysicsActor);
    }

    // ---- 초기화 ----
    /**
     * BeginPlay 전, 또는 SpawnActor 직후에 호출해 물리 씬과 연결.
     * @param Scene        물리 씬 (IPhysicsScene — 구현체 무관)
     * @param InitialTrans 초기 Transform (AActor와 동기화)
     */
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

    // ---- Unreal-style UActorComponent overrides ----
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        // RegisterWithPhysicsScene 는 반드시 BeginPlay 전에 호출되어야 함
    }

    /**
     * ★ Transform 동기화 핵심 ★
     * 물리 시뮬레이션 결과를 매 프레임 AActor Transform에 반영.
     * FixedUpdate(Simulate) 이후, 메인 Tick에서 호출됨.
     */
    virtual void TickComponent(float DeltaTime) override
    {
        Super::TickComponent(DeltaTime);

        if (!PhysicsActor || !PhysicsActor->IsSimulatingPhysics()) return;
        if (PhysicsActor->IsKinematic()) return;

        // 물리 엔진의 결과 Transform을 SceneComponent(= Actor의 Transform)에 복사
        FTransform PhysResult = PhysicsActor->GetTransform();
        SetTransform(PhysResult);
        // AActor::GetTransform()은 RootComponent(=this)를 읽으므로 자동 반영됨
    }

    // ---- 에디터 / 런타임 속성 접근자 ----
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
        // 등록 전에만 변경 가능 (등록 후 변경은 Remove → Recreate 필요)
    }

    /** 에디터에서 Transform 강제 변경 시 물리 액터에도 동기화 */
    void SyncTransformToPhysics(const FTransform& NewTransform)
    {
        SetTransform(NewTransform);
        if (PhysicsActor) PhysicsActor->SetTransform(NewTransform);
    }

    IPhysicsActor* GetPhysicsActor() const { return PhysicsActor; }

    /**
     * 백엔드 전환 직전 호출 — 씬/액터 포인터를 null로 초기화.
     * 기존 씬의 RemoveActor를 호출하지 않는다 (씬이 곧 파괴되므로).
     */
    void DetachFromPhysicsScene()
    {
        PhysicsScene = nullptr;
        PhysicsActor = nullptr;
    }

private:
    IPhysicsScene* PhysicsScene;   // 비소유 포인터 (씬이 더 오래 삶)
    IPhysicsActor* PhysicsActor;   // 비소유 포인터 (소유는 IPhysicsScene)

    bool    bSimulatePhysics;
    float   Mass;
    float   LinearDamping;
    FVector HalfExtents;

    using Super = USceneComponent;
};
