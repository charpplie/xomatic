//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File: AnimationManager.h
//  Implementation of Animation Manager.h
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#ifndef _CRYTEK_GAHLMG_
#define _CRYTEK_GAHLMG_

#include "GlobalAnimationHeader.h"
#include <NameCRCHelper.h>
#include <PoolAllocator.h>
extern stl::PoolAllocatorNoMT<sizeof(SAnimationSelectionProperties)> *g_Alloc_AnimSelectProps;


struct BSAnimationLMG
{
	Vec3  m_qVelocity;         //asset-feature: the velocity vector for this asset
	Vec3  m_Position;
	string m_strAnimName;

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_strAnimName);
	}
};

//////////////////////////////////////////////////////////////////////////
// this is the animation information on the module level (not on the per-model level)
// it doesn't know the name of the animation (which is model-specific), but does know the file name
// Implements some services for bone binding and ref counting
struct GlobalAnimationHeaderLMG : public GlobalAnimationHeader
{
	friend class CAnimationManager;
	friend class CAnimationSet;

	GlobalAnimationHeaderLMG ()
	{
		m_FilePathCRC32		=	0;
		m_nRef_by_Model		= 0;
		m_nBlendCodeLMG		=	0;
		m_nTouchedCounter	= 0;
		m_nSelectionCapsCode	=	0;
		m_pSelectionProperties = NULL;
	}

	const char* GetFilePath() const {	return m_FilePath.c_str(); };
	int GetFilePathCRC32() { return m_FilePathCRC32; }
	void SetFilePath(const string& name) 
	{ 
		m_FilePath = name; 
#ifdef _USE_LOWERCASE
		assert (name.size() < MAX_STATIC_CHARS);
		char pStringArray[MAX_STATIC_CHARS];
		NameCRCHelper::MakeLowercase(pStringArray, name.c_str());
		m_FilePathCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(pStringArray); 
#else
		m_FilePathCRC32 = gEnv->pSystem->GetCrc32Gen()->GetCRC32(m_Name.c_str()); 
#endif
	};


	void AllocateAnimSelectProps()
	{
		assert(m_pSelectionProperties == NULL);
		m_pSelectionProperties = (SAnimationSelectionProperties*)g_Alloc_AnimSelectProps->Allocate();
		m_pSelectionProperties->init();
	}

	virtual ~GlobalAnimationHeaderLMG()
	{
		if (m_pSelectionProperties != NULL)
			g_Alloc_AnimSelectProps->Deallocate(m_pSelectionProperties);
	};


	void AddRef()
	{
		++m_nRef_by_Model;
	}

	void Release()
	{
		if (!--m_nRef_by_Model)
		{
			m_arrBSAnimations.clear();	// nobody uses the blend-spaces; clean them up.
		}
	}



	size_t SizeOfLMG()const
	{
		size_t nSize = sizeof(*this);
		size_t nTemp00 = m_FilePath.capacity();		nSize += nTemp00;
		nSize += m_arrBSAnimations.get_alloc_size();

		nSize += m_AnimEventsLMG.get_alloc_size();
		uint32 numEvents = m_AnimEventsLMG.size();
		for (uint32 i=0; i<numEvents; i++)
		{
			nSize += m_AnimEventsLMG[i].m_strModelName.capacity();
			nSize += m_AnimEventsLMG[i].m_strEventName.capacity();
			nSize += m_AnimEventsLMG[i].m_strCustomParameter.capacity();
			nSize += m_AnimEventsLMG[i].m_strBoneName.capacity();
		}

		size_t s = m_strSpliceAnim.size();
		for(size_t i=0; i<s; ++i)
			nSize += (m_strSpliceAnim[i].length()+1);
		nSize += m_strSpliceAnim.capacity() * sizeof(string);

		nSize += m_jointList.get_alloc_size();

		return nSize;
	}

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject( m_FilePath );

		pSizer->AddObject( m_arrBSAnimations );
		pSizer->AddObject( m_AnimEventsLMG );		
		pSizer->AddObject( m_strSpliceAnim );
		pSizer->AddObject( m_jointList );		
	}
	
public:
	string m_FilePath;								//path-name - unique per-model
	uint32 m_FilePathCRC32;						//hash value for searching animations
	SAnimationSelectionProperties* m_pSelectionProperties;
	DynArray<BSAnimationLMG> m_arrBSAnimations;
	DynArray<string> m_strSpliceAnim;
	DynArray<AnimEvents> m_AnimEventsLMG;
	DynArray<uint32> m_jointList;

	// the number of referrers to this global animation record (doesn't matter if the controllers are currently loaded)
	int32  m_nRef_by_Model;
	uint32 m_nTouchedCounter;

	uint32 m_nBlendCodeLMG;
	uint32 m_nSelectionCapsCode;
} _ALIGN(16);


#endif
