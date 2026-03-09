#pragma once

#include "USceneComponent.h"

// Base class for anything that can be rendered or have collision
class UPrimitiveComponent : public USceneComponent
{
public:
    UPrimitiveComponent() = default;
    virtual ~UPrimitiveComponent() = default;

    // Interface for rendering - to be overridden by subclasses (e.g. UStaticMeshComponent)
    virtual void Render() {}
};
