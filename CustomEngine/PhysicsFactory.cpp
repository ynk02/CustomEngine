// PhysicsFactory.cpp
// 
// 이 파일이 구체 물리 Backend 구현체를 #include하는 유일한 파일.
// 새 백엔드(Jolt, PhysX)를 추가할 때 이 파일만 수정하면 됨.
//
// 디커플링 전략:
//   IPhysicsSystem.h   → 인터페이스만 선언, 구현체 몰라도 됨
//   NullPhysicsSystem.h → Null 구현체 (전체 헤더-only)
//   이 .cpp            → 팩토리 구현 (구현체 포함하는 유일한 TU)

#include "Physics/IPhysicsSystem.h"
#include "Physics/NullPhysicsSystem.h"
#include "Physics/AVBDPhysicsSystem.h"   // AVBD 구현체 (이 파일만 알면 됨)

// 추후 추가 예시:
// #ifdef USE_JOLT
// #include "Physics/JoltPhysicsSystem.h"
// #endif

std::unique_ptr<IPhysicsSystem> IPhysicsSystem::Create(EPhysicsBackend Backend)
{
    switch (Backend)
    {
    case EPhysicsBackend::Null:
        return std::make_unique<NullPhysicsSystem>();

    case EPhysicsBackend::AVBD:
        return std::make_unique<AVBDPhysicsSystem>();

    // 추후 Jolt 추가 시:
    // case EPhysicsBackend::Jolt:
    //     return std::make_unique<JoltPhysicsSystem>();

    default:
        return std::make_unique<NullPhysicsSystem>();
    }
}
