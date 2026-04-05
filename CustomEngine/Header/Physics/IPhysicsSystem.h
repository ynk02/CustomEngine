#pragma once

#include "IPhysicsScene.h"
#include <memory>

// ============================================================
//  IPhysicsSystem
//  물리 백엔드의 최상위 팩토리/매니저.
//  언리얼의 IPhysicsInterface 포지션.
//
//  사용 예시:
//    auto System = IPhysicsSystem::Create(EPhysicsBackend::Null);
//    auto Scene  = System->CreateScene();
//    Scene->Initialize();
//    ... (이후 IPhysicsScene/Actor 인터페이스만 사용)
// ============================================================

// 지원 물리 백엔드 목록
enum class EPhysicsBackend
{
    Null,    // 자체 AABB + Gravity stub (Simple Euler)
    AVBD,    // Adaptive Velocity-Based Dynamics (Gauss-Seidel Constraint Solver)
    Jolt,    // Jolt Physics (추후 구현)
    PhysX,   // NVIDIA PhysX (추후 구현)
};

class IPhysicsSystem
{
public:
    virtual ~IPhysicsSystem() = default;

    /** 시스템 초기화 */
    virtual bool Initialize() = 0;

    /** 시스템 종료 및 리소스 해제 */
    virtual void Shutdown() = 0;

    /**
     * 새 PhysicsScene 생성.
     * 여러 씬을 가질 수 있음 (예: 게임 씬 + 에디터 프리뷰 씬).
     */
    virtual std::unique_ptr<IPhysicsScene> CreateScene() = 0;

    // ---- Static Factory (RHI::CreateDynamicRHI() 대응) ----
    /**
     * Backend에 따라 구체 시스템 인스턴스를 생성.
     * main.cpp 등에서 단 한 번 호출 후 IPhysicsSystem 인터페이스로만 사용.
     */
    static std::unique_ptr<IPhysicsSystem> Create(EPhysicsBackend Backend = EPhysicsBackend::Null);
};
