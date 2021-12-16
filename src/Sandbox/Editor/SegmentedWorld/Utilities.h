#pragma once




SW_NAMESPACE_BEGIN();
//////////////////////////////////////////////////////////////////////////
// Naive Transaction
//
//	The transaction class that are declared in SW_TRANSACTION_BEGIN() must have two
//  methods: Commit() and Rollback()
//
// Usage:
//		SW_TRANSACTION_BEGIN(TransactionType, transObj)
//		{
//			...
//			if (bSomethingWrong)
//				SW_TRANSACTION_THROW();
//			...
//		}
//		SW_TRANSACTION_CATCH
//		{
//			...
//			transObj.Rollback();
//		}
//		SW_TRANSACTION_END;
//
//////////////////////////////////////////////////////////////////////////
#define SW_TRANSACTION_BEGIN(T,trans)		{ bool __bTransactionFailed = false; T trans; \
											T* __pCurrentTransaction = &trans; \
											do

#define SW_TRANSACTION_END					{__pCurrentTransaction->Commit();}}

// Write specific rollback code in this section
#define SW_TRANSACTION_CATCH		while(0); if (__bTransactionFailed)

// call this when the transaction should quit as failed & rollback
#define SW_TRANSACTION_THROW()		{__bTransactionFailed = true; break;}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Math
//////////////////////////////////////////////////////////////////////////
// Min base-2 powered number which is equal to or greater than n, the upper bound
inline uint32 UpperBase2PowerNum(uint32 n)
{
	--n;
	n |= n >> 16;
	n |= n >> 8;
	n |= n >> 4;
	n |= n >> 2;
	n |= n >> 1;
	++n;
	return n;
}
inline uint64 UpperBase2PowerNum(uint64 n)
{
	--n;
	n |= n >> 32;
	n |= n >> 16;
	n |= n >> 8;
	n |= n >> 4;
	n |= n >> 2;
	n |= n >> 1;
	++n;
	return n;
}

// Max base-2 powered number which is equal to or less than n, the lower bound, OR the Left-most Bit
template<typename T>
inline T LowerBase2PowerNum(T n)
{
	return UpperBase2PowerNum((n>>1)+1);
}

template<typename T>
inline T LeftMostBit(T n)
{
	return LowerBase2PowerNum(n);
}

template<typename T>
inline T RightMostBit(T n)
{
	return (~n +1) & n;
}

// custom point in bbox check: not inside if on max edge, ignoring z
inline bool ObjInBox(const AABB &bb, const Vec3 &pt)
{
	return pt.x >= bb.min.x && pt.x < bb.max.x && pt.y >= bb.min.y && pt.y < bb.max.y;
}

//////////////////////////////////////////////////////////////////////////
// LOG
//////////////////////////////////////////////////////////////////////////
void SWLog( const char * pszLog, ...);
void SWLogConsole( const char * pszLog, ...);
void SWLogV( bool bConsoleOnly,const char * pszLog, va_list args);

//////////////////////////////////////////////////////////////////////////
// Warning
//////////////////////////////////////////////////////////////////////////
void SWWarning( const char *format, ... );

//////////////////////////////////////////////////////////////////////////
// memory block save / load utility functions
//////////////////////////////////////////////////////////////////////////
bool SaveMemBlockToFile(const char *pcFileName, CMemoryBlock &mem, UINT nOpenFlags = 0);
bool LoadMemBlockFromFile(const char *pcFileName, CMemoryBlock &mem, UINT nOpenFlags = 0);
bool SaveRawDataToFile(const char* pcFileName, CMemoryBlock& mem);
bool SaveRawDataToFile(const char* pcFileName, void* pBuf, size_t cbSize);
bool SaveRawDataToScry(const char* pcFileName, CMemoryBlock& mem);
bool LoadRawDataFromFile(const char* pcFileName, CMemoryBlock& mem);
bool LoadRawDataFromScry(const char* pcFileName, CMemoryBlock& mem);


//////////////////////////////////////////////////////////////////////////
// "ask for proceeding" message box
//////////////////////////////////////////////////////////////////////////
enum EProceedMode {
	PROCEED_NO = 0,
	PROCEED_YES = 1,
	PROCEED_CANCEL = 2,
	PROCEED_ASK = 3
};

inline EProceedMode AskForProceeding(const char *pcMessage)
{
	int res = MessageBox(0, pcMessage, "Proceed?", MB_YESNOCANCEL | MB_ICONQUESTION);
	switch (res)
	{
	case IDYES: return PROCEED_YES;
	case IDNO: return PROCEED_NO;
	case IDCANCEL: return PROCEED_CANCEL;
	}
	assert(0);
	return PROCEED_ASK;
}


inline static void SplitGuid(REFGUID guid, int64 &nGUIDLo, int64 &nGUIDHi)
{
	nGUIDLo = guid.Data4[0];
	for (int i = 1; i < 8 ; i++)
		nGUIDLo = nGUIDLo<<8|guid.Data4[i];

	nGUIDHi = guid.Data1;
	nGUIDHi = nGUIDHi<<16|guid.Data2;
	nGUIDHi = nGUIDHi<<16|guid.Data3;
}

inline static void MakeGuid(GUID &guid, int64 nGUIDLo, int64 nGUIDHi)
{
	guid.Data1 = (nGUIDHi>>32)&0xFFFFFFFF;
	guid.Data2 = (nGUIDHi>>16)&0xFFFF;
	guid.Data3 = nGUIDHi&0xFFFF;
	for (int i = 0 ; i < 8; i++)
		guid.Data4[i] = (nGUIDLo>>8*(7-i))&0xFF;
}

bool UpdateFileToScryAndDelete(const CString& sPath);

bool CanAccessChilds(CBaseObject* pObject);
void GetBoundBoxWithChilds(CBaseObject* pObject, AABB &box);
TLocalCoords WorldToSegmentNoClamp(const Vec3 &v);
bool IsObjectBig(CBaseObject* pObject);

void SegIdToWorldRect(SegmentID nSegmentID, CRect &destWorldRect);
void SegIdToWorldRect(SegmentID nSegmentID, Recti &destWorldRect, int meterSize);

SW_NAMESPACE_END();
