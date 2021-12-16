#pragma once

#include "ReplayQueryFuture.h"
#include "IReplayTask.h"
#include "ReplayLogReader.h"

template <typename T>
class ReplayQuery : public IReplayQueryFuture<T>, public IReplayTask
{
public:
	typedef T ResultType;

public:
	ReplayQuery()
		: m_reader(NULL)
		, m_complete(false)
		, m_completeEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
		, m_result(T())
	{
	}

	~ReplayQuery()
	{
		CloseHandle(m_completeEvent);
	}

public: // IReplayQueryFuture Members
	T GetResult()
	{
		WaitForSingleObject(m_completeEvent, INFINITE);
		return m_result;
	}

	float GetProgress()
	{
		if (m_reader)
			return static_cast<float>(static_cast<double>(m_reader->GetPhysicalPosition()) / static_cast<double>(m_reader->GetPhysicalLength()));

		return m_complete ? 1.0f : 0.0f;
	}

	bool IsComplete()
	{
		return m_complete;
	}

public: // IReplayTask Members
	void Run(ReplayLogReader& reader)
	{
		m_reader = &reader;
		RunImpl(reader);
		m_reader = NULL;
	}

protected:
	void Complete(const T& result)
	{
		m_result = result;
		m_complete = true;
		SetEvent(m_completeEvent);
	}

private:
	virtual void RunImpl(ReplayLogReader& reader) = 0;

private:
	ReplayQuery(const ReplayQuery<T>&);
	ReplayQuery<T>& operator = (const ReplayQuery<T>&);

private:
	ReplayLogReader* m_reader;

	volatile bool m_complete;
	HANDLE m_completeEvent;
	T m_result;
};

