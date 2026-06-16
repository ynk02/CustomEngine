#pragma once

#include "IPhysicsScene.h"
#include <memory>

enum class EPhysicsBackend
{
    Null,
    XPBD,
    AVBD,
    Classical,
};

class IPhysicsSystem
{
public:
    virtual ~IPhysicsSystem() = default;

    virtual bool Initialize() = 0;

    virtual void Shutdown() = 0;

    virtual std::unique_ptr<IPhysicsScene> CreateScene() = 0;

    static std::unique_ptr<IPhysicsSystem> Create(EPhysicsBackend Backend = EPhysicsBackend::Null);
};
