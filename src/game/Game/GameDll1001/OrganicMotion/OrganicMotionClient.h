
#ifndef OM_CLIENT_H
#define OM_CLIENT_H

//#if defined(PS3) || defined(LINUX) || defined(WIN64) || defined(XENON)
#define DISABLE_OM 1
//#endif

struct SDefaultJoint
{
	int32 m_Primary;
	int32 m_idxParent;
	const char* m_bname;
	QuatT	m_DefRelativeQuat;			
	QuatT	m_DefAbsoluteQuat;			

	QuatT	m_RelativeQuat;			
	QuatT	m_AbsoluteQuat;			

	ILINE SDefaultJoint()
	{
		m_Primary=0;
		m_idxParent=-1;
		m_bname=0;
		m_DefRelativeQuat.SetIdentity();			
		m_DefAbsoluteQuat.SetIdentity();			

		m_RelativeQuat.SetIdentity();			
		m_AbsoluteQuat.SetIdentity();			
	};

	ILINE SDefaultJoint( int32 prim, int32 pidx, const char* bname, QuatT qt) 
	{
		m_Primary			=	prim;
		m_idxParent		=	pidx;
		m_bname				=	bname;
		m_DefRelativeQuat=	qt;
	};
};

//////////////////////////////////////////////////////////////////////////
#define MAX_NAMESIZE 40
typedef char namestring[MAX_NAMESIZE];

#include "sdkclientinterfaces.h"
#include "SDKClientFactory.h"

#define NUM_KEYS 2

//////////////////////////////////////////////////////////////////////////
typedef union _LARGE_INTEGER2
{
	struct
	{
		unsigned long LowPart;
		int HighPart;
	};
	struct
	{
		unsigned long LowPart;
		int HighPart;
	} u;
	long long QuadPart;
} LARGE_INTEGER2;

//////////////////////////////////////////////////////////////////////////
typedef struct tOrganicMotionClient
{
	ISDKClient * pClient;
	IOMBoneSetupInfo * pBoneSetupInfo;
	IStreamDataFrame * pFrame;

	int * viBoneParentList;
	OMBoneDimension * vdimBoneDims;
	namestring * vstrBoneNames;
	OMBoneMatrix * pBoneMats;

	int iNumBones;
	int iBoneMatsSize;

	bool	bConnected;

	OMDataKey keys[NUM_KEYS];

	LARGE_INTEGER2 timestampFreq;
	LARGE_INTEGER2 timestamp;
	LARGE_INTEGER2 lastTimestamp;
	long long timeStampSave;

	void ExternalPostProcessing(ICharacterInstance* pInstance);

}tOrganicMotionClient;

#endif
