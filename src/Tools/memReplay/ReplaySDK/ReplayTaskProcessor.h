#pragma once

#include "IReplayTask.h"
#include "ReplayLogReader.h"

#include "CriticalSection.h"

class CWinThread;

class ReplayTaskProcessor
{
public:
	explicit ReplayTaskProcessor(const SharedPtr<ReplayLogReader>& logReader);
	~ReplayTaskProcessor();

	void QueueTask(const SharedPtr<IReplayTask>& task);

private:
	ReplayTaskProcessor(const ReplayTaskProcessor&);
	ReplayTaskProcessor& operator = (const ReplayTaskProcessor&);

private:
	static unsigned int __stdcall ThreadProxy(void* param)
	{
		return reinterpret_cast<ReplayTaskProcessor*>(param)->Thread();
	}

private:
	unsigned int Thread();

private:
	SharedPtr<ReplayLogReader> m_logReader;

	CriticalSection m_tasksLock;
	std::deque<SharedPtr<IReplayTask> > m_tasks;

	uintptr_t m_thread;
};
