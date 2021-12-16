#include "stdafx.h"
#include "ReplayLogProgressTask.h"

ReplayLogProgressTask::ReplayLogProgressTask(ReplayLogReader& reader, IReplayListener& listener)
	: m_reader(&reader)
	, m_innerListener(&listener)
	, m_progress(0)
{
}

void ReplayLogProgressTask::Start()
{
	m_reader->Replay(*this);
}

int ReplayLogProgressTask::GetProgress() const
{
	return m_progress;
}

int ReplayLogProgressTask::GetMaximumProgress() const
{
	return 10000;
}

void ReplayLogProgressTask::ReplayBegin()
{
	m_innerListener->ReplayBegin();
}

void ReplayLogProgressTask::Replay(ReplayRange range)
{
	m_innerListener->Replay(range);

	LONG progress = static_cast<LONG>((10000 * m_reader->GetPhysicalPosition()) / m_reader->GetPhysicalLength());
	InterlockedExchange(&m_progress, progress);
}

void ReplayLogProgressTask::ReplayEnd(u64 position)
{
	m_innerListener->ReplayEnd(position);

	InterlockedExchange(&m_progress, 10000);
}
