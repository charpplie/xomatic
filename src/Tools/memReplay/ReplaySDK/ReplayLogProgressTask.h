#pragma once

#include "IInterfaceHooks.h"
#include "ReplayLogReader.h"

class ReplayLogProgressTask : public IProgressTask, private IReplayListener
{
public:
	ReplayLogProgressTask(ReplayLogReader& reader, IReplayListener& listener);

	void Start();

	int GetProgress() const;
	int GetMaximumProgress() const;

private:
	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

private:
	ReplayLogReader* m_reader;
	IReplayListener* m_innerListener;

	volatile LONG m_progress;
};

