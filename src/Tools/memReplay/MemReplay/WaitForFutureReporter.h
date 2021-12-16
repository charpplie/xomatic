#pragma once

template <typename T>
class WaitForFutureReporter : public IProgressReporter
{
public:
	WaitForFutureReporter(ReplayQueryFuture<T> future)
		: m_future(future)
	{}

	int GetProgress() const
	{
		return std::min(
			static_cast<int>(m_future->GetProgress() * 1000.0f),
			m_future->IsComplete() ? 1000 : 999);
	}

	int GetMaximumProgress() const { return 1000; }

private:
	ReplayQueryFuture<T> m_future;
};

template <typename T>
WaitForFutureReporter<T> MakeWaitForFutureReporter(ReplayQueryFuture<T> future)
{
	return WaitForFutureReporter<T>(future);
}
