#pragma once

class IPhysicsCCD
{

public:
	virtual ~IPhysicsCCD() = default;

	virtual void SetCCDEnabled(bool bEnabled) = 0;

	virtual bool IsCCDEnabled() const = 0;

	virtual void SetCCDMaxSubSteps(int MaxSubSteps) = 0;

	virtual int GetCCDMaxSubSteps() const = 0;

};
