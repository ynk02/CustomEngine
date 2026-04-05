#pragma once

#include "CoreMinimal.h"

// -----------------------------------------------------------------------
// FRay
// Screen-to-world ray used for mouse picking.
// -----------------------------------------------------------------------
struct FRay
{
    FVector Origin;
    FVector Direction;
};
