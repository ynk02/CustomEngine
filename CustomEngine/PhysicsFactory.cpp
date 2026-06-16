#include "Physics/IPhysicsSystem.h"
#include "Physics/NullPhysicsSystem.h"
#include "Physics/XPBDPhysicsSystem.h"
#include "Physics/ClassicalPhysicsSystem.h"
#include "Physics/AVBDPhysicsSystem.h"

std::unique_ptr<IPhysicsSystem> IPhysicsSystem::Create(EPhysicsBackend Backend)
{
    switch (Backend)
    {
    case EPhysicsBackend::Null:
        return std::make_unique<NullPhysicsSystem>();

    case EPhysicsBackend::XPBD:
        return std::make_unique<XPBDPhysicsSystem>();

    case EPhysicsBackend::Classical:
        return std::make_unique<ClassicalPhysicsSystem>();

    case EPhysicsBackend::AVBD:
        return std::make_unique<AVBDPhysicsSystem>();

    default:
        return std::make_unique<NullPhysicsSystem>();
    }
}
