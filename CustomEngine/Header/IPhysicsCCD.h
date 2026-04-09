#pragma once





class IPhysicsCCD
{

public:
	virtual ~IPhysicsCCD() = default;

	virtual void SetCCDEnabled(bool bEnabled) = 0;

	virtual bool IsCCDEnabled() const = 0;




};