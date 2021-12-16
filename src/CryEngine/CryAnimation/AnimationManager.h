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

#ifndef _CRYTEK_CONTROLLER_MANAGER_HEADER_
#define _CRYTEK_CONTROLLER_MANAGER_HEADER_


#include "GlobalAnimationHeaderLMG.h"
#include "GlobalAnimationHeaderPMG.h"
#include "GlobalAnimationHeaderCAF.h"
#include "GlobalAnimationHeaderAIM.h"

class CAnimationSet;
class CryCGALoader;
class CAnimationManager;
class CAnimationSet;
class CCommonSkinningInfo;
struct CGlobalHeaderDBA;



/////////////////////////////////////////////////////////////////////////////////////////////////////
// class CAnimationManager 
// Responsible for creation of multiple animations, subsequently bind to the character bones
// There is one instance of this class per a game.
/////////////////////////////////////////////////////////////////////////////////////////////////////
class CAnimationManager : public IAnimEvents
{
public:
	void Clear();

	void StreamOnComplete (IReadStream* pStream, unsigned nError);
	//! Returns the number of installed anim-events for this asset
	int32 GetAnimEventsCount_CAF(int nGlobalID ) const; 
	int32 GetAnimEventsCount_CAF(const char* pFilePath );
	void AddAnimEvent_CAF(int nGlobalID, const char* pName, const char* pParameter, const char* pBone, float fTime, const Vec3& vOffset, const Vec3& vDir, const char* pModel);
	void AddAnimEvent_LMG(int nGlobalID, const char* pName, const char* pParameter, const char* pBone, float fTime, const Vec3& vOffset, const Vec3& vDir, const char* pModel);
	void AddAnimEvent_PMG(int nGlobalID, const char* pName, const char* pParameter, const char* pBone, float fTime, const Vec3& vOffset, const Vec3& vDir, const char* pModel);

	void DeleteAllEventsForCAF(int nGlobalID);
	void DeleteAllEventsForLMG(int nGlobalID);
	void DeleteAllEventsForPMG(int nGlobalID);

	int GetGlobalAnimID_CAF(const char* pFilePath);
	int GetGlobalAnimID_LMG(const char* pFilePath);
	int GetGlobalAnimID_PMG(const char* pFilePath);

	// finds the animation by name. Returns -1 if no animation was found. Returns the animation ID if it was found
	int GetGlobalIDbyFilePath_CAF( const char* sFilePath );
	int GetGlobalIDbyFilePath_AIM( const char* sFilePath );
	int GetGlobalIDbyFilePath_LMG( const char* sFilePath );
	int GetGlobalIDbyFilePath_PMG( const char* sFilePath );
	// create header with the specified name. If the header already exists, it return the index
	int CreateGAH_CAF( const string& strFileName );
	int CreateGAH_AIM( const string& strFileName );
	int CreateGAH_LMG( const string& strFileName );
	int CreateGAH_PMG( const string& strFileName );

	// loads existing animation record, returns false on error
	bool InitGAHCAF_fromDBA( GlobalAnimationHeaderCAF& rGAH, bool bSuppressWarning);
	bool InitGAHCAF_fromCAF( GlobalAnimationHeaderCAF& rGAH, eLoadingOptions flags, bool bAsync);
	bool LoadAnimationTCB(int nGlobalAnimId, std::vector<CControllerTCB>& m_LoadCurrAnimation, CryCGALoader* pCGA, uint32 unique_model_id  );
	bool InitGAHAIM_fromAIM( GlobalAnimationHeaderAIM& rAIM);

	void UnloadAnimationCAF(int nGLobalAnimID);
	void UnloadAnimationAIM(int nGLobalAnimID);

	// returns the total number of animations hold in memory (for statistics)
	size_t GetGlobalAnimCount()  { return m_arrGlobalCAF.size();	}

	// notifies the controller manager that this client doesn't use the given animation any more.
	// these calls must be balanced with AnimationAddRef() calls
	void AnimationRelease (int nGlobalAnimId, CAnimationSet* pClient);

	// puts the size of the whole subsystem into this sizer object, classified, according to the flags set in the sizer
	void GetSize(class ICrySizer* pSizer);
	void GetMemoryUsage(class ICrySizer* pSizer) const;	
	
	void Register (CAnimationSet* pClient)
	{
		m_arrClients.insert (std::lower_bound(m_arrClients.begin(), m_arrClients.end(), pClient), pClient);
	}
	void Unregister (CAnimationSet* pClient)
	{
		DynArray<CAnimationSet*>::iterator it = std::lower_bound(m_arrClients.begin(), m_arrClients.end(), pClient);
		if(it != m_arrClients.end() && *it == pClient)
			m_arrClients.erase (it);
		else
			assert (0); // the unregistered client tries to unregister
	}


	size_t GetSizeOfDBA();
	bool CreateGlobalHeaderDBA(DynArray<string>& name);
	bool IsDatabaseInMeory(uint32 nDBACRC32);

	void RemoveSkinningInfo_(const char* filename);

private:

public:
	DynArray<CGlobalHeaderDBA*> m_arrGlobalHeaderDBA;

	CNameCRCMap		m_AnimationMapCAF;
	DynArray<GlobalAnimationHeaderCAF> m_arrGlobalCAF;
	//	CNameCRCMap		m_AnimationMapAIM;
	DynArray<GlobalAnimationHeaderAIM> m_arrGlobalAIM;
	//	CNameCRCMap		m_AnimationMapLMG;
	DynArray<GlobalAnimationHeaderLMG> m_arrGlobalLMG;
	//	CNameCRCMap		m_AnimationMapPMG;
	DynArray<GlobalAnimationHeaderPMG> m_arrGlobalPMG;

	// the sorted list of pointers to the clients that use the services of this manager
	DynArray<CAnimationSet*> m_arrClients;




	CAnimationManager(const CAnimationManager&);
	void operator=(const CAnimationManager&);

	CAnimationManager()
	{ 
		g_Alloc_AnimSelectProps = new stl::PoolAllocatorNoMT<sizeof(SAnimationSelectionProperties)>();
		// we reserve the place for future animations. The best performance will be
		// archived when this number is >= actual number of animations that can be used, and not much greater
		m_arrGlobalCAF.reserve(3300); 
		m_arrGlobalAIM.reserve(140);
		m_arrGlobalLMG.reserve(130);
		m_arrGlobalPMG.reserve( 0); 
	}

	~CAnimationManager()
	{
		delete g_Alloc_AnimSelectProps;
	}


private:

};													 


#endif
