#pragma once

template <typename T>
class IReplayQueryFuture
{
public:
	virtual ~IReplayQueryFuture() {}

	virtual T GetResult() = 0;

	virtual float GetProgress() = 0;
	virtual bool IsComplete() = 0;
};

template <typename T> struct ReplayQueryFuture : public SharedPtr<IReplayQueryFuture<T> >
{
	ReplayQueryFuture() {}
	ReplayQueryFuture(IReplayQueryFuture<T>* ptr) : SharedPtr<IReplayQueryFuture<T> >(ptr) {}
	ReplayQueryFuture(const SharedPtr<IReplayQueryFuture<T> >& other) : SharedPtr<IReplayQueryFuture<T> >(other) {}

	T GetResult() { return (*this)->GetResult(); }
	float GetProgress() { return (*this)->GetProgress(); }
	bool IsComplete() { return (*this)->IsComplete(); }
};

template <typename T>
class ReplayImmediateFuture : public IReplayQueryFuture<T>
{
public:
	ReplayImmediateFuture(const T& result)
		: m_result(result)
	{
	}

	T GetResult() { return m_result; }
	float GetProgress() { return 1.0f; }
	bool IsComplete() { return true; }

private:
	T m_result;
};
