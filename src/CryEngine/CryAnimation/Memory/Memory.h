#ifndef Memory_h
#define Memory_h

namespace Memory {

class CPool;

class CContext
{
public:
	CContext();

public:
	void AddPool(CPool& pool);
	void RemovePool(CPool& pool);

	bool HasPool(CPool& pool);

	void Update();

	void GetMemoryUsage(ICrySizer* pSizer) const;

private:
	CPool* m_pPools;
};

} // namespace Memory

class CAnimationContext
{
public:
	static CAnimationContext& Instance()
	{
		static CAnimationContext instance;
		return instance;
	}

public:
	Memory::CContext& GetMemoryContext() { return m_memoryContext; }

	void Update();

	void GetMemoryUsage(ICrySizer* pSizer) const;

private:
	Memory::CContext m_memoryContext;
};

#endif // Memory_h
