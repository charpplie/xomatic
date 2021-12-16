#pragma once

class ReplayLogReader;

class IReplayTask
{
public:
	virtual ~IReplayTask() {}

	virtual void Run(ReplayLogReader& reader) = 0;
};
