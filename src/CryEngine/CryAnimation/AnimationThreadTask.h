#ifndef AnimationThreadTask_h
#define AnimationThreadTask_h

class CAnimationThreadEvent
{
public:
	CAnimationThreadEvent() :
		m_lock(1)
	{
	}

public:
	ILINE void Reset()
	{
		m_lock = 1;
	}

	ILINE void Set()
	{
		m_lockNotify.Lock();
		m_lock = 0;
		m_lockCondition.Notify();
		m_lockNotify.Unlock();
	}

	ILINE void Wait()
	{
		m_lockNotify.Lock();
		while(*(volatile int*)&m_lock)
			m_lockCondition.Wait(m_lockNotify);
		m_lockNotify.Unlock();
	}

private:
	int m_lock;
	CryMutex m_lockNotify;
	CryConditionVariable m_lockCondition;
};

class CAnimationThreadTask :
	public IThreadTask
{
public:
	void Begin();

public:
	virtual void Execute() = 0;

	// IThreadTask
private:
	virtual void OnUpdate();
	virtual void Stop() { }
	virtual struct SThreadTaskInfo* GetTaskInfo() { return &m_threadTaskInfo; }

public:
	SThreadTaskInfo m_threadTaskInfo;
};

class CAnimationThreadPool
{
public:
	static CAnimationThreadPool& Instance()
	{
		static CAnimationThreadPool instance;
		return instance;
	}

private:
	friend class CAnimationThread;

private:
	CAnimationThreadPool();

public:
	bool Initialize();
	void AddTask(CAnimationThreadTask* pTask);

private:
	ThreadPoolHandle m_hThreadPool;
};

#endif // AnimationThreadTask_h
