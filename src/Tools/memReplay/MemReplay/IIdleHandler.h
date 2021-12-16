#pragma once

class IIdleHandler
{
public:
	virtual ~IIdleHandler() {}

	virtual void OnIdle() = 0;
};
