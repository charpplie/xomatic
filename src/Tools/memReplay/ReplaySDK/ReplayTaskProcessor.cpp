#include "stdafx.h"
#include "ReplayTaskProcessor.h"

#include <process.h>

ReplayTaskProcessor::ReplayTaskProcessor(const SharedPtr<ReplayLogReader>& logReader)
	: m_logReader(logReader)
{
}

ReplayTaskProcessor::~ReplayTaskProcessor()
{
}

void ReplayTaskProcessor::QueueTask(const SharedPtr<IReplayTask>& task)
{
	size_t count;

	{
		CriticalSectionLock lock(m_tasksLock);
		count = m_tasks.size();
		m_tasks.push_back(task);
	}

	if (count == 0)
	{
		m_thread = _beginthreadex(NULL, 0, ThreadProxy, this, 0, NULL);
	}
}

unsigned int ReplayTaskProcessor::Thread()
{
	bool running = true;

	while (running)
	{
		SharedPtr<IReplayTask> task;

		{
			CriticalSectionLock lock(m_tasksLock);
			if (!m_tasks.empty())
			{
				task = m_tasks.front();
				m_tasks.pop_front();
			}
			else
			{
				running = false;
			}
		}

		if (task.IsValid())
		{
			task->Run(*m_logReader);
		}
	}

	return 0;
}
