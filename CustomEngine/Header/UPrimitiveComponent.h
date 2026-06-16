#pragma once

#include "USceneComponent.h"

class UPrimitiveComponent : public USceneComponent
{
public:
    UPrimitiveComponent() = default;
    virtual ~UPrimitiveComponent() = default;

    virtual void Render() {}
};
