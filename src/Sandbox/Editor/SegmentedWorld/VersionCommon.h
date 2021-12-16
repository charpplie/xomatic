#ifndef __VersionCommon_h__
#define __VersionCommon_h__

#if _MSC_VER > 1000
#pragma once
#endif



SW_NAMESPACE_BEGIN();
// forward declares
struct Versioned;

//version types
enum EVersionType {
	//VT_OFFICIAL = 0,
	VT_CURRENT,
	//VT_HOT,

	VT_COUNT,
	VT_UNSPECIFIED = -1,
};
extern const char* const g_pcVersionTypes[sw::VT_COUNT];

enum ELockResult
{
	LockResult_Unspecified = -1,
	LockResult_Failed = 0,
	LockResult_AlreadyHave,
	LockResult_Succeeded,
	LockResult_Hot,
	LockResult_Offline, // In offline mode, so don't need to lock
};

enum EDataType
{
	DataType_WorldDataBlock = 0,
	DataType_Segment,
	DataType_Layer,
	DataType_Object,
	DataType_Count,
};

// TODO: all of these specializing should be removed later!! -RL
enum EVersionedDataType
{
	VDT_WORLD,
	VDT_SEGTRUNK,
	VDT_LAYER,
	VDT_EXTOBJECT,
	VDT_SEGTRUNK_COMP_HEAD,
	VDT_SEGTRUNK_COMP_BODY,
	VDT_COUNT
};

// special history record IDs
static const int64 HRID_MISSING = 0;
static const int64 HRID_UNKNOWN = -1;
static const int64 HRID_HEADREV = 0x7FffFFffFFffFFff;
//static const int64 HRID_CACHE = -2;
//static const int64 HRID_EDITOR = -3;

struct TCheckOutContext
{
	int64 m_nRev;
	int64 m_nChangeList;
	sw::EVersionType m_vt;

	TCheckOutContext()
	{
		Clear();
	}
	explicit TCheckOutContext(int64 hrid, int64 nChangeList, sw::EVersionType vt)
		:m_nRev(hrid)
		,m_nChangeList(nChangeList)
		,m_vt(vt)
	{}

	operator bool() const
	{
		return m_nRev > sw::HRID_MISSING;
	}
	void Clear()
	{
		m_nRev = sw::HRID_MISSING;
		m_vt = sw::VT_UNSPECIFIED;
		m_nChangeList = -1;
	}
};

SW_NAMESPACE_END();


#endif // __VersionCommon_h__