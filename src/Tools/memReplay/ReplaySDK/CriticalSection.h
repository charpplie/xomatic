#pragma once

class CriticalSection
{
public:
	CriticalSection()
	{
		InitializeCriticalSection(&m_lock);
	}

	~CriticalSection()
	{
		DeleteCriticalSection(&m_lock);
	}

	void Enter()
	{
		EnterCriticalSection(&m_lock);
	}

	void Leave()
	{
		LeaveCriticalSection(&m_lock);
	}

private:
	CriticalSection(const CriticalSection&);
	CriticalSection& operator = (const CriticalSection&);

private:
	CRITICAL_SECTION m_lock;
};

class CriticalSectionLock
{
public:
	explicit CriticalSectionLock(CriticalSection& cs)
		: m_cs(&cs)
	{
		m_cs->Enter();
	}

	~CriticalSectionLock()
	{
		m_cs->Leave();
	}

private:
	CriticalSectionLock(const CriticalSectionLock&);
	CriticalSectionLock& operator = (const CriticalSectionLock&);

private:
	CriticalSection* m_cs;
};

class SpinCriticalSection
{
public:
	SpinCriticalSection(DWORD spinCount)
	{
		InitializeCriticalSectionAndSpinCount(&m_cs, spinCount);
	}

	~SpinCriticalSection()
	{
		DeleteCriticalSection(&m_cs);
	}

	void Enter()
	{
		EnterCriticalSection(&m_cs);
	}

	void Leave()
	{
		LeaveCriticalSection(&m_cs);
	}

private:
	SpinCriticalSection(const SpinCriticalSection&);
	SpinCriticalSection& operator = (const SpinCriticalSection&);

private:
	CRITICAL_SECTION m_cs;
};

class SpinCriticalSectionLock
{
public:
	explicit SpinCriticalSectionLock(SpinCriticalSection& cs)
		: m_cs(&cs)
	{
		m_cs->Enter();
	}

	~SpinCriticalSectionLock()
	{
		m_cs->Leave();
	}

private:
	SpinCriticalSectionLock(const CriticalSectionLock&);
	SpinCriticalSectionLock& operator = (const SpinCriticalSectionLock&);

private:
	SpinCriticalSection* m_cs;
};
