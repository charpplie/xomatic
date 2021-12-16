#ifndef Pool_h
#define Pool_h

namespace Memory {

class CContext;

//

class CPool
{
private:
	friend class CContext;

protected:
	CPool(CContext& context);
	virtual ~CPool();

public:
	void Release() { delete this; }

	virtual void* Allocate(uint32 length) { return NULL; }
	virtual void Free(void* pAddress) { }

	virtual void Update() { }

	virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

private:
	CContext* m_pContext;
	CPool* m_pNext;
};

class CPoolFrameLocal :
	public CPool
{
private:
	class CBucket
	{
	public:
		static CBucket* Create(uint32 length);

	private:
		CBucket(uint32 length);
		~CBucket();

	public:
		void Release() { delete this; }

		void Reset();
		void* Allocate(uint32 length);

		ILINE void GetMemoryUsage(ICrySizer* pSizer) const;

	private:
		CBucket(const CBucket&);
		CBucket& operator = (const CBucket&);

	private:
		uint8* m_buffer;
		uint32 m_bufferSize;
		uint32 m_length;
		uint32 m_used;
	};

public:
	static CPoolFrameLocal* Create(CContext& context, uint32 bucketLength);

private:
	CPoolFrameLocal(CContext& context, uint32 bucketLength);
	~CPoolFrameLocal();

public:
	void ReleaseBuckets();

	void Reset();

private:
	CBucket* CreateBucket();

	// CPool
public:
	virtual void* Allocate(uint32 length);

	virtual void Update();

	virtual void GetMemoryUsage(ICrySizer* pSizer) const;

private:
	uint32 m_bucketLength;
	std::vector<CBucket*> m_buckets;
	uint32 m_bucketCurrent;
};

ILINE void CPoolFrameLocal::CBucket::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_buffer);
}

} // namespace Memory

#endif // Pool_h
