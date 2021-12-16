#ifndef __CRYDXPSAPTR__
#define __CRYDXPSAPTR__

#ifdef WIN32
	#pragma warning( disable : 4355) 
#endif

extern void WaitFinishThread(const bool);

template<class T>
class CCryRefAndWeak
{
private:
	uint16							m_RefCount;
	uint16							m_WeakCount;
public:

											CCryRefAndWeak() :
											m_RefCount(1),
											m_WeakCount(0)
											{
											}

	unsigned long				IncRef()
											{
												return ++m_RefCount;
											}

	unsigned long				DecRef()
											{
												if(!--m_RefCount)
													if(static_cast<T*>(this)->Locked())
													{
														static_cast<T*>(this)->FreeMe();
													}
													else
													{
														static_cast<T*>(this)->ReleaseResources();
														if(!m_WeakCount)
														{
															CRY_DXPS_RELEASE(static_cast<T*>(this));
															return 0; // don't access m_RefCount after delete this 
														}
													}
												return m_RefCount;
											}

	void								AddWeak()
											{
												++m_WeakCount;
											}
	void								ReleaseWeak()
											{
												if(!--m_WeakCount && !m_RefCount)
												{
													if(static_cast<T*>(this)->Locked())
														static_cast<T*>(this)->FreeMe();
												}
												else
												{
													CRY_DXPS_RELEASE(static_cast<T*>(this));
												}
											}
	uint32							Valid()	const
											{
												return m_RefCount;
											}
};

template<class T>
class CCryAPtrRefCnt
{
	T*									m_pData;
public:
											CCryAPtrRefCnt():
											m_pData(0)
											{
											}
											CCryAPtrRefCnt(T* pData):
											m_pData(pData)
											{
#ifdef CRY_DXPS_DESTRUCTORVALIDATION
												if(pData->Type() == EDXPS_RT_NONE)
												{
													snPause();
												}
#endif
												pData->IncRef();
											}
											CCryAPtrRefCnt(const CCryAPtrRefCnt<T>& rData):
											m_pData(rData->m_pData)
											{
#ifdef CRY_DXPS_DESTRUCTORVALIDATION
												if(rData->m_pData->Type() == EDXPS_RT_NONE)
												{
													snPause();
												}
#endif
												rData->IncRef();
											}
											~CCryAPtrRefCnt()
											{
												if(m_pData)
													m_pData->DecRef();
											}

	CCryAPtrRefCnt<T>&	operator=(T* pData)
											{
#ifdef CRY_DXPS_DESTRUCTORVALIDATION
												if(pData && pData->Type() == EDXPS_RT_NONE)
												{
													snPause();
												}
#endif
												if(pData)
													pData->IncRef();
												if(m_pData)
													m_pData->DecRef();
												m_pData	=	pData;
												return *this;
											}

	CCryAPtrRefCnt<T>&	operator=(const CCryAPtrRefCnt<T>& rData)
											{
#ifdef CRY_DXPS_DESTRUCTORVALIDATION
												if(rData->Type() == EDXPS_RT_NONE)
												{
													snPause();
												}
#endif
												if(rData->m_pData)
													rData->m_pData->IncRef();
												if(m_pData)
													m_pData->DecRef();
												m_pData	=	rData->m_pData;
												return *this;
											}

	T*									operator->()
											{
												return m_pData;
											}

											operator T*()
											{
												return m_pData;
											}
};

template<class T>
class CCryAPtrWeakCnt
{
	T*									m_pData;
public:
											CCryAPtrWeakCnt():
											m_pData(0)
											{
											}
											CCryAPtrWeakCnt(T* pData):
											m_pData(pData)
											{
#ifdef CRY_DXPS_DESTRUCTORVALIDATION
												if(pData->Type() == EDXPS_RT_NONE)
												{
													snPause();
												}
#endif
												pData->AddWeak();
											}
											CCryAPtrWeakCnt(const CCryAPtrWeakCnt<T>& rData):
											m_pData(rData->m_pData)
											{
#ifdef CRY_DXPS_DESTRUCTORVALIDATION
												if(rData->Type() == EDXPS_RT_NONE)
												{
													snPause();
												}
#endif
												rData->AddWeak();
											}
											~CCryAPtrWeakCnt()
											{
												if(m_pData)
													m_pData->ReleaseWeak();
											}

	CCryAPtrWeakCnt<T>&	operator=(T* pData)
											{
#ifdef CRY_DXPS_DESTRUCTORVALIDATION
												if(pData && pData->Type() == EDXPS_RT_NONE)
												{
													snPause();
												}
#endif
												if(pData)
													pData->AddWeak();
												if(m_pData)
													m_pData->ReleaseWeak();
												m_pData	=	pData;
												return *this;
											}

	CCryAPtrWeakCnt<T>&	operator=(const CCryAPtrWeakCnt<T>& rData)
											{
#ifdef CRY_DXPS_DESTRUCTORVALIDATION
												if(rData->m_pData && rData->m_pData->Type() == EDXPS_RT_NONE)
												{
													snPause();
												}
#endif
												if(rData->m_pData)
													rData->m_pData->AddWeak();
												if(m_pData)
													m_pData->ReleaseWeak();
												m_pData	=	rData->m_pData;
												return *this;
											}

	T*									operator->()
											{
												return m_pData && m_pData->Valid()?m_pData:0;
											}

	const T*						operator->() const
											{
												return m_pData && m_pData->Valid()?m_pData:0;
											}

	operator						T*()
											{
												return m_pData && m_pData->Valid()?m_pData:0;
											}
	operator const			T*()	const
											{
												return m_pData && m_pData->Valid()?m_pData:0;
											}
};

template<class T,bool ARRAY=true>
class	CCryAPtrScope
{
	T*									m_pData;

											CCryAPtrScope(const CCryAPtrScope<T>& rData){}
	void								operator=(const CCryAPtrScope<T>& rData){}

public:
											CCryAPtrScope():
											m_pData(0)
											{
											}
											CCryAPtrScope(T* pData):
											m_pData(pData)
											{
											}
											~CCryAPtrScope()
											{
												if(ARRAY)
													delete[] m_pData;
												else
													delete m_pData;
											}

	void								Release()
											{
												if(ARRAY)
													delete[] m_pData;
												else
													delete m_pData;
												m_pData	=	0;
											}

	T*									operator->()
											{
												return m_pData;
											}

											operator T*()
											{
												return m_pData;
											}
};

template<class T>
class CCryAPtrBoundCheck
{
	T*									m_pData;
	uint32							m_Size;
 
											CCryAPtrBoundCheck(const CCryAPtrBoundCheck<T>& rData)
											{
											}

	void								operator=(const CCryAPtrBoundCheck<T>& rData)
											{
											}

public:
											CCryAPtrBoundCheck(T* pData,uint32 Size):
											m_pData(pData),
											m_Size(Size)
											{
											}
											~CCryAPtrBoundCheck()
											{
													delete m_pData;
											}

	T&									operator[](uint32 Pt)
											{
												assert(Pt<m_Size);
												return m_pData[Pt];
											}
	const T&						operator[](uint32 Pt)const
											{
												assert(Pt<m_Size);
												return m_pData[Pt];
											}
};

#endif

