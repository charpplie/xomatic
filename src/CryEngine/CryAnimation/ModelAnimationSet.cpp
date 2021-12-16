////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	10/9/2004 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  interface class to all motions  
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CharacterManager.h"
#include "Model.h"
#include "ModelAnimationSet.h"
#include "ModelMesh.h"
#include "crc32.h"
#include "LMG.h"
#include "SDI.h"
#include "LoaderDBA.h"

#include "LoaderLMG.h"

// on spu a structure defined in forwardkinematicsspu.cpp is used to hold the needed data
#if !defined(__SPU__)
typedef GlobalAnimationHeaderCAF&	GlobalAnimationHeaderType;
#endif

CAnimationSet::CAnimationSet()
{
	m_pModel=0;
	m_CharEditMode=0;
}

CAnimationSet::~CAnimationSet()
{
	// now there are no controllers referred to by this object, we can release the animations
	for (DynArray<ModelAnimationHeader>::iterator it = m_arrAnimations.begin(); it != m_arrAnimations.end(); ++it)
		if (it->m_nAssetType==CAF_File)
			g_AnimationManager.AnimationRelease(it->m_nGlobalAnimId, this);

	g_AnimationManager.Unregister(this);
}

void CAnimationSet::Init()
{
	g_AnimationManager.Register(this);
}

size_t CAnimationSet::SizeOfAnimationSet() const
{
	size_t nSize=sizeof(CAnimationSet);
	uint32 numAnimations = m_arrAnimations.size();
	for (uint32 i=0; i<numAnimations; i++)
		nSize += m_arrAnimations[i].SizeOfAnimationHeader();

	// somebody know how to get size of multimap?
	nSize += m_FullPathAnimationsMap.size() * 4* sizeof(uint32) + m_AnimationHashMap.GetAllocMemSize();

	//	nSize += m_arrAnimByGlobalId.size() * sizeof(LocalAnimId);
	nSize += m_arrStandupAnims.size() * sizeof(int);

	for (FacialAnimationSet::const_iterator it = m_facialAnimations.begin(), end = m_facialAnimations.end();	it != end; ++it) 
	{
		nSize += it->name.capacity(); 
		nSize += it->path.capacity();
	}
	return nSize;
}

void CAnimationSet::GetMemoryUsage(ICrySizer *pSizer) const
{
	pSizer->AddObject( m_arrAnimations );	
	pSizer->AddObject( m_arrStandupAnimTypes );	
	pSizer->AddObject( m_arrStandupAnims );
	pSizer->AddObject(m_AnimationHashMap);
}



int CAnimationSet::ReloadCAF(const char * szFileName, bool bFullPath)
{
	int nModelAnimId;// = GetIDByName(szFileName);

	if (bFullPath)
	{
		uint32 crc = g_pCrc32Gen->GetCRC32Lowercase(szFileName);

		std::pair<TCRCmap::iterator, TCRCmap::iterator> p = m_FullPathAnimationsMap.equal_range(crc);
		TCRCmap::iterator it = p.first;//*m_FullPathAnimationsMap.find(crc);*/
		TCRCmap::iterator end = p.second; //m_FullPathAnimationsMap.end();

		int res = -1;
		for (; it != end; ++it)
		{
			res = StreamCAF(it->second,0);
		}

		if( res >=0 ) 
			CryLog( "Hot loaded animation CAF file: %s", PathUtil::GetFileName(szFileName).c_str() );

		return res;
	}

	nModelAnimId = GetAnimIDByName(szFileName);

	if (nModelAnimId == -1)
		return -1;

	int res = StreamCAF(nModelAnimId,0);
	if( res >=0 ) 
		CryLog( "Hot loaded animation CAF file: %s", PathUtil::GetFileName(szFileName).c_str() );

	return res;
}


int CAnimationSet::StreamCAF(int nGlobalAnimID, bool asynch)
{
	GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];
	uint32 numController = rCAF.GetControllersCount();
	assert(numController==0); //little security check  
	assert(rCAF.IsAssetOnDemand());

	//streaming is probbaly the only case where we need to store the full file-path
	const char* pFilePath = rCAF.GetFilePath();

	g_AnimationManager.InitGAHCAF_fromCAF(rCAF, eLoadFullData, asynch);
	return nGlobalAnimID;
}


int CAnimationSet::RenewCAF(const char * szFileName, bool bFullPath)
{
	int nModelAnimId;// = GetIDByName(szFileName);

	if (bFullPath)
	{
		uint32 crc = g_pCrc32Gen->GetCRC32Lowercase(szFileName);
		std::pair<TCRCmap::iterator, TCRCmap::iterator> p = m_FullPathAnimationsMap.equal_range(crc);
		TCRCmap::iterator it = p.first;//*m_FullPathAnimationsMap.find(crc);*/
		TCRCmap::iterator end = p.second; //m_FullPathAnimationsMap.end();

		int res = -1;
		for (; it != end; ++it)
		{
			res = RenewCAF(it->second);
		}

		return res;
	}

	nModelAnimId = GetAnimIDByName(szFileName);

	if (nModelAnimId == -1)
		return -1;

	return RenewCAF(nModelAnimId);

}


int CAnimationSet::RenewCAF(int nGlobalAnimID)
{
	return nGlobalAnimID;
}

const char* CAnimationSet::GetFacialAnimationPathByName(const char* szName)
{
	FacialAnimationSet::iterator itFacialAnim = std::lower_bound(m_facialAnimations.begin(), m_facialAnimations.end(), szName, stl::less_stricmp<const char*>());
	if (itFacialAnim != m_facialAnimations.end() && stl::less_stricmp<const char*>()(szName, *itFacialAnim))
		itFacialAnim = m_facialAnimations.end();
	const char* szPath = (itFacialAnim != m_facialAnimations.end() ? (*itFacialAnim).path.c_str() : 0);
	return szPath;
}

int CAnimationSet::GetNumFacialAnimations()
{
	return m_facialAnimations.size();
}

const char* CAnimationSet::GetFacialAnimationName(int index)
{
	if (index < 0 || index >= (int)m_facialAnimations.size())
		return 0;
	return m_facialAnimations[index].name.c_str();
}




const char* CAnimationSet::GetFacialAnimationPath(int index)
{
	if (index < 0 || index >= (int)m_facialAnimations.size())
		return 0;
	return m_facialAnimations[index].path.c_str();
}




//////////////////////////////////////////////////////////////////////////
// Loads animation file. Returns the global anim id of the file, or -1 if error
// SIDE EFFECT NOTES:
//  THis function does not put up a warning in the case the animation couldn't be loaded.
//  It returns an error (false) and the caller must process it.
int CAnimationSet::LoadCAF(const char* szFilePath, const char* szAnimName)
{
	//NEW RULE: every single CAF file is now an OnDemand streaming animation
	int nAnimId = GetAnimIDByName (szAnimName);
	if (nAnimId != -1)
	{
		int nGlobalAnimID = m_arrAnimations[nAnimId].m_nGlobalAnimId;
		g_pILog->LogError("CryAnimation:: Trying to load animation with alias \"%s\" from file \"%s\" into the animation container. Such animation alias already exists and uses file \"%s\". Please use another animation alias.", szAnimName, szFilePath, g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].GetFilePath());
		return nGlobalAnimID;
	}

	int nGlobalAnimID = g_AnimationManager.CreateGAH_CAF(szFilePath);

	ModelAnimationHeader localAnim; 
	localAnim.m_nGlobalAnimId = nGlobalAnimID;
	localAnim.m_nAssetType=CAF_File;
	localAnim.SetAnimName(szAnimName);
	m_arrAnimations.push_back(localAnim);

	int nLocalAnimId2 = m_arrAnimations.size()-1;
	m_AnimationHashMap.InsertValue(&localAnim, nLocalAnimId2);


	string path = szFilePath;
	path.replace('\\','/' );
	int p = path.find("//");
	while (p != string::npos)
	{
		path.erase(p,1 );
		p = path.find("//");
	}
	path.MakeLower();
	uint32 crc = g_pCrc32Gen->GetCRC32Lowercase(path.c_str());
//Diesel cut	
	//m_FullPathAnimationsMap.insert(std::make_pair<uint32, int32>(crc,nGlobalAnimID));

//Diesel new
	m_FullPathAnimationsMap.insert(std::make_pair(crc, nGlobalAnimID));


	GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];
	rCAF.AddRef();

	uint32 IsCreated = rCAF.IsAssetCreated();
	if (IsCreated==0)
	{
		//	if (rCAF.m_bInitializedByAIF)
		//		return nGlobalAnimID;

		uint32 IsAimPose = (CryStringUtils::stristr(szAnimName,"AimPoses") != 0);
		//CRY_ASSERT_TRACE(!IsAimPose, ("we don't load aim-poses here: %s",szFilePath));
		if (IsAimPose)
			CryFatalError("we don't load aim-poses here: %s",szFilePath);

		//--------------------------------------------------------------------------------

		assert(rCAF.GetControllersCount()==0);


		/*
		uint8 status = g_AnimationManager.InitGAH_fromCAF(rCAF,eLoadFullData, 0);
		if (status==0)
		{
		rCAF.ClearAssetNotFound();
		g_AnimationManager.InitGAHCAF_fromDBA( g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID], false);
		}
		else
		{
		rCAF.OnAssetOnDemand();
		//	rCAF.ClearAssetProcessed();
		//	rCAF.ClearAssetRequested();
		//	rCAF.ClearAssetLoaded();
		//		assert(rCAF.GetControllersCount()==0);
		//	assert(rCAF.IsAssetLoaded()==0);
		g_AnimationManager.UnloadAnimationCAF(nGlobalAnimID);
		}*/

		if (1)
		{
			if (g_Consoleself.ca_LoadPriority) 
			{
				rCAF.ClearAssetNotFound();
				if (!g_AnimationManager.InitGAHCAF_fromDBA( g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID], true)) 
				{
					uint8 status = g_AnimationManager.InitGAHCAF_fromCAF(rCAF, eLoadOnlyInfo, 0);
					if (status)
					{
						assert(rCAF.GetControllersCount()==0);
						rCAF.OnAssetOnDemand();
						rCAF.ClearAssetProcessed();
						rCAF.ClearAssetRequested();
						rCAF.ClearAssetLoaded();
						assert(rCAF.GetControllersCount()==0);
						assert(rCAF.IsAssetLoaded()==0);
					}
				}
			} 
			else 
			{
				uint8 status = g_AnimationManager.InitGAHCAF_fromCAF(rCAF, eLoadOnlyInfo, 0);
				if (status)
				{
					assert(rCAF.GetControllersCount()==0);
					rCAF.OnAssetOnDemand();
					rCAF.ClearAssetProcessed();
					rCAF.ClearAssetRequested();
					rCAF.ClearAssetLoaded();
					assert(rCAF.GetControllersCount()==0);
					assert(rCAF.IsAssetLoaded()==0);
				}
				else
				{
					rCAF.ClearAssetNotFound();
					g_AnimationManager.InitGAHCAF_fromDBA( rCAF, false);
				}
			}
		}

	}

	if (!_strnicmp(szAnimName,"standup",7))
	{
		const char *ptr;
		char buf[256];
		int i;
		for(ptr=szAnimName+8; *ptr && _strnicmp(ptr,"back",4) && _strnicmp(ptr,"stomach",7) && _strnicmp(ptr,"side",4); ptr++);
		strncpy(buf, szAnimName+8, ptr-szAnimName-8); buf[ptr-szAnimName-8]=0;
		for(i=0; i<(int)m_arrStandupAnimTypes.size() && _stricmp(m_arrStandupAnimTypes[i],buf)<0; i++);
		if (i>=(int)m_arrStandupAnimTypes.size() || _stricmp(m_arrStandupAnimTypes[i],buf)) 
		{
			m_arrStandupAnimTypes.insert(m_arrStandupAnimTypes.begin()+i, buf);
			m_arrStandupAnims.insert(m_arrStandupAnims.begin()+i, std::vector<int>());
		}
		m_arrStandupAnims[i].push_back(nLocalAnimId2);
	}


	return nGlobalAnimID;
}






//////////////////////////////////////////////////////////////////////////
// Loads animation file. Returns the global anim id of the file, or -1 if error
// SIDE EFFECT NOTES:
//  THis function does not put up a warning in the case the animation couldn't be loaded.
//  It returns an error (false) and the caller must process it.
int CAnimationSet::LoadAIM(const char* szFilePath, const char* szAnimName)
{
	//NEW RULE: every single AIM file is now an OnDemand streaming animation
	int nAnimId = GetAnimIDByName (szAnimName);
	if (nAnimId != -1)
	{
		int nGlobalAnimID = m_arrAnimations[nAnimId].m_nGlobalAnimId;
		g_pILog->LogError("CryAnimation:: Trying to load animation with alias \"%s\" from file \"%s\" into the animation container. Such animation alias already exists and uses file \"%s\". Please use another animation alias.", szAnimName, szFilePath, g_AnimationManager.m_arrGlobalAIM[nGlobalAnimID].GetFilePath());
		return nGlobalAnimID;
	}

	int nGlobalAnimID = g_AnimationManager.CreateGAH_AIM(szFilePath);


	ModelAnimationHeader localAnim; 
	localAnim.m_nGlobalAnimId = nGlobalAnimID;
	localAnim.m_nAssetType=AIM_File;
	localAnim.SetAnimName(szAnimName);
	m_arrAnimations.push_back(localAnim);


	int nLocalAnimId2 = m_arrAnimations.size()-1;
	m_AnimationHashMap.InsertValue(&localAnim, nLocalAnimId2);


	string path = szFilePath;
	path.replace('\\','/' );
	int p = path.find("//");
	while (p != string::npos)
	{
		path.erase(p,1 );
		p = path.find("//");
	}
	path.MakeLower();


	uint32 crc = g_pCrc32Gen->GetCRC32Lowercase(path.c_str());
//Diesel cut	
	//m_FullPathAnimationsMap.insert(std::make_pair<uint32, int32>(crc,nGlobalAnimID));
//Diesel new
	m_FullPathAnimationsMap.insert(std::make_pair(crc, nGlobalAnimID));


	GlobalAnimationHeaderAIM& rAIM = g_AnimationManager.m_arrGlobalAIM[nGlobalAnimID];
	rAIM.AddRef();

	uint32 loaded = rAIM.IsAssetCreated();
	if (loaded)
	{
		/*
		uint8 status = g_AnimationManager.InitGAHAIM_fromAIM(rAIM);
		if (status)
		{
			uint32 nAnimTokenCRC32 = 0;
			int32 nDirectionIdx = -1;
			uint32 numDB = m_pModel->m_ModelSkeleton.m_DirectionalBlends.size();
			for (uint32 d=0; d<numDB; d++)
			{
				const char* strAnimToken = m_pModel->m_ModelSkeleton.m_DirectionalBlends[d].m_AnimToken;
				uint32 IsAIM = (CryStringUtils::stristr(szAnimName,strAnimToken) != 0);
				if (IsAIM)
				{
					nAnimTokenCRC32 = m_pModel->m_ModelSkeleton.m_DirectionalBlends[d].m_AnimTokenCRC32;
					nDirectionIdx = m_pModel->m_ModelSkeleton.m_DirectionalBlends[d].m_nParaJointIdx;
					break;
				}
			}
			ProcessAimPoses(m_pModel,szAnimName,nGlobalAnimID,nDirectionIdx,nAnimTokenCRC32);
		}*/
		return nGlobalAnimID;
	}
	uint32 IsAimPose = (CryStringUtils::stristr(szAnimName,"AimPoses") != 0);
	//CRY_ASSERT_TRACE(IsAimPose, ("we don't load normal assets here: %s",szFilePath));
	//if (IsAimPose==0)
	//	CryFatalError("we don't load normal-assets here: %s",szFilePath);

	//----------------------------------------------------------------------------------------

	assert(rAIM.GetControllersCount()==0);
	uint8 status = g_AnimationManager.InitGAHAIM_fromAIM(rAIM);
	if (status)
	{
		uint32 nAnimTokenCRC32 = 0;
		int32 nDirectionIdx = -1;
		uint32 numDB = m_pModel->m_ModelSkeleton.m_DirectionalBlends.size();
		for (uint32 d=0; d<numDB; d++)
		{
			const char* strAnimToken = m_pModel->m_ModelSkeleton.m_DirectionalBlends[d].m_AnimToken;
			uint32 IsAIM = (CryStringUtils::stristr(szAnimName,strAnimToken) != 0);
			if (IsAIM)
			{
				nAnimTokenCRC32 = m_pModel->m_ModelSkeleton.m_DirectionalBlends[d].m_AnimTokenCRC32;
				nDirectionIdx = m_pModel->m_ModelSkeleton.m_DirectionalBlends[d].m_nParaJointIdx;
				break;
			}
		}

		ProcessAimPoses(m_pModel,szAnimName,rAIM,nDirectionIdx,nAnimTokenCRC32);
		if (Console::GetInst().ca_UnloadAimPoses)
		{
			g_AnimationManager.UnloadAnimationAIM(nGlobalAnimID);
			rAIM.OnAimposeUnloaded();
		}
	} 
	else
	{
		//Aim-Pose does not exist as file. this is an error
		const char* pPathName=rAIM.GetFilePath();
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE,pPathName,	"Failed to load animation CAF file" );
	} 

	return nGlobalAnimID;
}





//////////////////////////////////////////////////////////////////////////
// Loads animation file. Returns the global anim id of the file, or -1 if error
// SIDE EFFECT NOTES:
//  THis function does not put up a warning in the case the animation couldn't be loaded.
//  It returns an error (false) and the caller must process it.
int CAnimationSet::LoadANM(const char* szFileName, const char* szAnimName, std::vector<CControllerTCB>& m_LoadCurrAnimation, CryCGALoader* pCGA, uint32 unique_model_id )
{
	int nModelAnimId = GetAnimIDByName (szAnimName);
	if (nModelAnimId == -1) 
	{
		int nGlobalAnimID = g_AnimationManager.CreateGAH_CAF(szFileName);

		int nLocalAnimId = m_arrAnimations.size();
		ModelAnimationHeader LocalAnim; 
		LocalAnim.m_nGlobalAnimId = nGlobalAnimID;
		LocalAnim.m_nAssetType=CAF_File;
		LocalAnim.SetAnimName(szAnimName);
		m_arrAnimations.push_back(LocalAnim);

		//m_arrAnimByGlobalId.insert (std::lower_bound(m_arrAnimByGlobalId.begin(), m_arrAnimByGlobalId.end(), nGlobalAnimID, AnimationGlobIdPred(m_arrAnimations)), LocalAnimId(nLocalAnimId));
		m_AnimationHashMap.InsertValue(&LocalAnim, nLocalAnimId);
		//		m_arrAnimByLocalName.insert (std::lower_bound(m_arrAnimByLocalName.begin(), m_arrAnimByLocalName.end(), szAnimName, AnimationNamePred(m_arrAnimations)), nLocalAnimId);

		g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].AddRef();

		uint32 loaded = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].IsAssetLoaded();
		if (loaded)
		{
			//assert(g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].m_arrController.size());
			assert(g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].m_arrController);
		}
		else
		{
			//assert(g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].m_arrController.size()==0);
			//assert(g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].m_arrController);
			g_AnimationManager.LoadAnimationTCB(nGlobalAnimID, m_LoadCurrAnimation, pCGA, unique_model_id );
		}

		return nGlobalAnimID;
	}
	else
	{
		int nGlobalAnimID = m_arrAnimations[nModelAnimId].m_nGlobalAnimId;
		g_pILog->LogError("CryAnimation:: Trying to load animation with alias \"%s\" from file \"%s\" into the animation container. Such animation alias already exists and uses file \"%s\". Please use another animation alias.",szAnimName,	szFileName,	g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID].GetFilePath());
		return nGlobalAnimID;
	}
}





//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

int CAnimationSet::LoadLMG(const char* szFilePath, const char* szAnimName)
{
	int nModelAnimId = GetAnimIDByName (szAnimName);
	if (nModelAnimId == -1) 
	{
		int nGlobalAnimID = g_AnimationManager.CreateGAH_LMG(szFilePath);
		int nLocalAnimId = m_arrAnimations.size();

		ModelAnimationHeader LocalAnim; 
		LocalAnim.m_nGlobalAnimId = nGlobalAnimID;
		LocalAnim.m_nAssetType=LMG_File;
		LocalAnim.SetAnimName(szAnimName);
		m_arrAnimations.push_back(LocalAnim);

		m_AnimationHashMap.InsertValue(&LocalAnim, nLocalAnimId);

		g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID].AddRef();

		uint32 loaded=g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID].IsAssetCreated();//Loaded();
		if (loaded==0)
		{
			//assert(g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID].IsAssetLMG()==0);
			assert(g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID].m_arrBSAnimations.size()==0);
			GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID];
			rGlobalAnimHeader.OnAssetLMG();
			bool r=LMG::ParseXML( rGlobalAnimHeader ); 
			if (r)
			{
				rGlobalAnimHeader.OnAssetCreated();
				rGlobalAnimHeader.OnAssetLoaded();
			}
			else
			{
				rGlobalAnimHeader.OnAssetNotFound();
				AnimFileWarning( szFilePath, "XML-File for LMG not found" );
			}
		}
		return nGlobalAnimID;
	}
	else
	{
		int nGlobalAnimID = m_arrAnimations[nModelAnimId].m_nGlobalAnimId;
		g_pILog->LogError("CryAnimation:: Trying to load animation with alias \"%s\" from file \"%s\" into the animation container. Such animation alias already exists and uses file \"%s\". Please use another animation alias.",szAnimName, szFilePath,	g_AnimationManager.m_arrGlobalLMG[nGlobalAnimID].GetFilePath());
		return nGlobalAnimID;
	}

}


int CAnimationSet::CreateOrSetAnimationLMG( const char* animationName, int globalAnimationLmgId )
{
	int localAnimationId = GetAnimIDByName( animationName );

	bool animationAlreadyLoaded = ( localAnimationId != -1 );
	if ( animationAlreadyLoaded )
	{
		ModelAnimationHeader& animationHeader = m_arrAnimations[ localAnimationId ];
		int oldGlobalAnimationId = animationHeader.m_nGlobalAnimId;
		bool changingAnimationGlobalId = ( oldGlobalAnimationId != globalAnimationLmgId );
		if ( changingAnimationGlobalId )
		{
			animationHeader.m_nGlobalAnimId = globalAnimationLmgId;

			g_AnimationManager.m_arrGlobalLMG[ globalAnimationLmgId ].AddRef();
			g_AnimationManager.m_arrGlobalLMG[ oldGlobalAnimationId ].Release();
		}
	}
	else
	{
		localAnimationId = m_arrAnimations.size();

		ModelAnimationHeader animationHeader; 
		animationHeader.SetAnimName( animationName );
		animationHeader.m_nGlobalAnimId = globalAnimationLmgId;	
		animationHeader.m_nAssetType = LMG_File;

		g_AnimationManager.m_arrGlobalLMG[ globalAnimationLmgId ].AddRef();

		m_arrAnimations.push_back( animationHeader );

		m_AnimationHashMap.InsertValue( &m_arrAnimations[ localAnimationId ], localAnimationId );

	}

	return localAnimationId;
}

//------------------------------------------------------------------------------
int CAnimationSet::LoadLMGFromMemory( const char* resourceName, const char* szAnimName, const char* xmlData )
{
	CLoaderLMG loader( this );
	LMGLoadResult result = loader.Load( resourceName, szAnimName, xmlData );
	if ( result == LMG_LOAD_SUCCESS )
	{
		int32 animationID = GetAnimIDByName( szAnimName );
		VerifyLocomotionGroup( animationID );
	}

	return ( result == LMG_LOAD_SUCCESS )? 0 : -1;
}

//------------------------------------------------------------------------------
bool CAnimationSet::Reading_XML_for_PMG(  GlobalAnimationHeaderPMG& rGlobalAnim ) 
{
	uint32 numAnims = rGlobalAnim.m_arrBSAnimationsPMG.size();
	if (numAnims >0)
		return 1; //already loaded

	const char* pathname = rGlobalAnim.GetFilePath(); 

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_LMG, 0, "%s", pathname);

	g_pILog->LogToFile("Loading PMG %s", pathname);				// to file only so console is cleaner
	g_pILog->UpdateLoadingScreen(0);
	XmlNodeRef root		= g_pISystem->LoadXmlFile(pathname);	
	if (root==0)
	{
		g_pILog->LogError ("Parameterized locomotion-group not found: %s", pathname);
		return 0;
	}

	const char* XMLTAG = root->getTag();
	rGlobalAnim.m_paramSpecPMG.clear();
	rGlobalAnim.m_arrBSAnimationsPMG.clear();
	rGlobalAnim.m_edgeList.clear();
	rGlobalAnim.m_virtualExmps.clear();
	rGlobalAnim.m_paramNamePMGMap.clear();
	rGlobalAnim.m_userParam.clear();

	//------------------------------------------------------------------------------
	// PMG
	if (strcmp(XMLTAG,"ParameterizedMotionGroup")==0) 
	{
		rGlobalAnim.m_arrBSAnimationsPMG.reserve( MAX_LMG_ANIMS );

		uint32 numChilds = root->getChildCount();
		for (uint32 c=0; c<numChilds; c++)
		{
			XmlNodeRef nodeList = root->getChild(c);
			const char* ListTag = nodeList->getTag();


			//-----------------------------------------------------------
			// Parameter extraction
			//-----------------------------------------------------------
			if ( strcmp(ListTag,"ParameterExtraction")==0 ) 
			{
				uint32 num = nodeList->getChildCount();
				for (uint32 i=0; i<num; i++) 
				{
					XmlNodeRef nodeExample = nodeList->getChild(i);
					const char* ExampleTag = nodeExample->getTag();
					GlobalAnimationHeaderPMG::SParamPMG param;
					if (strcmp(ExampleTag, "PARAM")==0) 
					{
						BSAnimationPMG bsa;
						const char* p = NULL;
						param.name = nodeExample->getAttr("Name");
						param.locatorJoint = nodeExample->getAttr("Joint");

						nodeExample->getAttr( "StartKey", param.startKey );
						nodeExample->getAttr( "EndKey",  param.endKey );
						nodeExample->getAttr( "MinValue", param.minValue );
						nodeExample->getAttr( "MaxValue", param.maxValue );
						if( nodeExample->haveAttr("Scale") )
							nodeExample->getAttr( "Scale", param.scale );
						else
							param.scale = 1.0f;

						rGlobalAnim.m_paramSpecPMG.push_back(param);
						rGlobalAnim.m_paramNamePMGMap[param.name] = rGlobalAnim.m_paramSpecPMG.size() -1;
					}
					else 
						return 0;
				}
				continue;
			}


			//-----------------------------------------------------------
			// Load example-list 
			//-----------------------------------------------------------
			uint32 paraNum = (uint32)rGlobalAnim.m_paramNamePMGMap.size();
			if ( strcmp(ListTag,"ExampleList")==0 ) 
			{
				rGlobalAnim.m_arrBSAnimationsPMG.clear();
				uint32 num = nodeList->getChildCount();	
				rGlobalAnim.m_userParam.resize(num);

				for (uint32 i=0; i<num; i++) 
				{
					XmlNodeRef nodeExample = nodeList->getChild(i);
					const char* ExampleTag = nodeExample->getTag();
					if (strcmp(ExampleTag,"Example")==0) 
					{
						BSAnimationPMG bsa;
						bsa.m_strAnimName = nodeExample->getAttr( "AName" );
						rGlobalAnim.m_arrBSAnimationsPMG.push_back(bsa);

						for(uint32 j=0; j<paraNum; ++j) // User parameters
						{
							if(nodeExample->haveAttr(rGlobalAnim.m_paramSpecPMG[j].name.c_str()))
							{
								GlobalAnimationHeaderPMG::SUserParam p;
								p.name = rGlobalAnim.m_paramSpecPMG[j].name;
								nodeExample->getAttr(p.name.c_str(), p.val); 
								rGlobalAnim.m_userParam[i].push_back(p);
							}
						}
					}
					else 
						return 0;
				}
				continue;
			}

			if ( strcmp(ListTag,"EdgeList")==0 ) 
			{
				rGlobalAnim.m_edgeList.clear();
				uint32 num = nodeList->getChildCount();
				for (uint32 i=0; i<num; i++) 
				{
					XmlNodeRef nodeExample = nodeList->getChild(i);
					const char* ExampleTag = nodeExample->getTag();
					if (strcmp(ExampleTag,"Edge")==0) 
					{
						GlobalAnimationHeaderPMG::SVirtualEdge examp;
						nodeExample->getAttr( "v1", examp.ind1);
						nodeExample->getAttr( "v2", examp.ind2);
						rGlobalAnim.m_edgeList.push_back(examp);
					}
					else 
						return 0;
				}
				continue;
			}

			if ( strcmp(ListTag,"SubdivideEdgeList")==0 ) 
			{
				rGlobalAnim.m_subdivideEdgeList.clear();
				uint32 num = nodeList->getChildCount();
				for (uint32 i=0; i<num; i++) 
				{
					XmlNodeRef nodeExample = nodeList->getChild(i);
					const char* ExampleTag = nodeExample->getTag();
					if (strcmp(ExampleTag,"Edge")==0) 
					{
						GlobalAnimationHeaderPMG::SVirtualEdge examp;
						nodeExample->getAttr( "v1", examp.ind1);
						nodeExample->getAttr( "v2", examp.ind2);
						rGlobalAnim.m_subdivideEdgeList.push_back(examp);
					}
					else 
						return 0;
				}
				continue;
			}

			if ( strcmp(ListTag,"VirtualExampleList")==0 ) 
			{
				rGlobalAnim.m_virtualExmps.clear();
				uint32 num = nodeList->getChildCount();
				for (uint32 i=0; i<num; i++) 
				{
					XmlNodeRef nodeExample = nodeList->getChild(i);
					const char* ExampleTag = nodeExample->getTag();
					if (strcmp(ExampleTag,"VirtualExample")==0) 
					{
						GlobalAnimationHeaderPMG::SVirtualExamp examp;
						nodeExample->getAttr( "v1", examp.ind1);
						nodeExample->getAttr( "v2", examp.ind2);
						nodeExample->getAttr( "alpha", examp.alpha);

						rGlobalAnim.m_virtualExmps.push_back(examp);
					}
					else 
						return 0;
				}
				continue;
			}


		}
	}

	return 1;
}


//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

int CAnimationSet::LoadPMG(const char* szFilePath, const char* szAnimName)
{
	int nModelAnimId = GetAnimIDByName (szAnimName);
	if (nModelAnimId == -1) 
	{
		int nGlobalAnimID = g_AnimationManager.CreateGAH_PMG(szFilePath);
		int nLocalAnimId = m_arrAnimations.size();

		ModelAnimationHeader LocalAnim; 
		LocalAnim.m_nGlobalAnimId = nGlobalAnimID;
		LocalAnim.m_nAssetType=PMG_File;
		LocalAnim.SetAnimName(szAnimName);
		m_arrAnimations.push_back(LocalAnim);

		m_AnimationHashMap.InsertValue(&LocalAnim, nLocalAnimId);

		g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].AddRef();

		uint32 loaded=g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].IsAssetCreated();//Loaded();
		if (loaded==0)
		{
			assert(g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].IsAssetLMG()==0);
			assert(g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].m_arrBSAnimationsPMG.size()==0);
			GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID];
			rGlobalAnimHeader.OnAssetPMG();
			bool r=Reading_XML_for_PMG( rGlobalAnimHeader ); 
			if (r)
			{
				rGlobalAnimHeader.OnAssetCreated();
				rGlobalAnimHeader.OnAssetLoaded();
			}
			else
			{
				rGlobalAnimHeader.OnAssetNotFound();
				assert( !"XML-File for PMG not found" );
			}
		}
		return nGlobalAnimID;
	}
	else
	{
		int nGlobalAnimID = m_arrAnimations[nModelAnimId].m_nGlobalAnimId;
		g_pILog->LogError("CryAnimation:: Trying to load animation with alias \"%s\" from file \"%s\" into the animation container. Such animation alias already exists and uses file \"%s\". Please use another animation alias.",szAnimName, szFilePath,	g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].GetFilePath());
		return nGlobalAnimID;
	}

}

int CAnimationSet::CreateDummyPMG()
{
	const char* szFilePath = "";
	const char* szAnimName = "_PMG_Dummy";
	int nModelAnimId = GetAnimIDByName (szAnimName);
	assert(nModelAnimId == -1);
	if (nModelAnimId == -1) 
	{
		int nGlobalAnimID = (int)g_AnimationManager.m_arrGlobalPMG.size();
		g_AnimationManager.m_arrGlobalPMG.push_back(GlobalAnimationHeaderPMG()); //CreateGAH_PMG(0,szFilePath);
		g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].SetFilePath("");
		//	g_AnimationManager.m_AnimationMapPMG.InsertValue(g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].GetFilePathCRC32(), nGlobalAnimID);
		int nLocalAnimId = m_arrAnimations.size();

		ModelAnimationHeader LocalAnim; 
		LocalAnim.m_nGlobalAnimId = nGlobalAnimID;
		LocalAnim.m_nAssetType=PMG_File;
		LocalAnim.SetAnimName(szAnimName);
		m_arrAnimations.push_back(LocalAnim);

		m_AnimationHashMap.InsertValue(&LocalAnim, nLocalAnimId);

		//g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].AddRef();

		uint32 loaded=g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].IsAssetCreated();//Loaded();
		assert(g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].IsAssetLMG()==0);
		assert(g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID].m_arrBSAnimationsPMG.size()==0);
		GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[nGlobalAnimID];
		rGlobalAnimHeader.SetFilePath("_PMG_Dummy_PATH");

		rGlobalAnimHeader.OnAssetPMG();
		BSAnimationPMG animName;
		animName.m_strAnimName = "combat_run_rifle_left_fast_03";
		rGlobalAnimHeader.m_arrBSAnimationsPMG.push_back(animName);
		animName.m_strAnimName = "combat_run_rifle_right_fast_03";
		rGlobalAnimHeader.m_arrBSAnimationsPMG.push_back(animName);
		rGlobalAnimHeader.OnAssetCreated();
		rGlobalAnimHeader.OnAssetLoaded();

		// Create PMG parameter
		GlobalAnimationHeaderPMG::SParamPMG param;
		param.name = "Blend Weight";
		param.minValue = 0.0f;
		param.maxValue = 1.0f;

		rGlobalAnimHeader.m_paramSpecPMG.push_back(param);
		rGlobalAnimHeader.m_paramNamePMGMap[param.name] = rGlobalAnimHeader.m_paramSpecPMG.size() -1;

		return nGlobalAnimID;
	}
	return -1;
}

//--------------------------------------------------------------------------------


const SAnimationSelectionProperties* CAnimationSet::GetAnimationSelectionProperties(const char* szAnimationName)
{
	int32 localAnimID = GetAnimIDByName(szAnimationName);
	if (localAnimID < 0)
		return NULL;

	// Just make sure we have the prop's computed...
	if (!ComputeSelectionProperties(localAnimID))
		return NULL;

	int32 globalAnimID = m_arrAnimations[localAnimID].m_nGlobalAnimId;

	if (m_arrAnimations[localAnimID].m_nAssetType==CAF_File)
		return g_AnimationManager.m_arrGlobalCAF[globalAnimID].m_pSelectionProperties;
	if (m_arrAnimations[localAnimID].m_nAssetType==LMG_File)
		return g_AnimationManager.m_arrGlobalLMG[globalAnimID].m_pSelectionProperties;

	CryFatalError("CryAnimation: Invalid Anim-Type");

	assert(0);
	return 0;
}




bool CAnimationSet::ComputeSelectionProperties(int32 localAnimID)
{
	int32 globalAnimID = m_arrAnimations[localAnimID].m_nGlobalAnimId;



	if (m_arrAnimations[localAnimID].m_nAssetType==CAF_File)
	{
		uint32 num = g_AnimationManager.m_arrGlobalCAF.size();
		assert(int32(num)>globalAnimID);
		GlobalAnimationHeaderCAF& globalAnimHeaderCAF = g_AnimationManager.m_arrGlobalCAF[globalAnimID];
		if (globalAnimHeaderCAF.m_pSelectionProperties != NULL)
			return true;
		if (!globalAnimHeaderCAF.IsAssetCreated())
			return false;
		/*
		uint32 OnDemand=globalAnimHeaderCAF.IsAssetOnDemand();
		if (OnDemand)
		{
		uint32 IsLoaded=globalAnimHeaderCAF.IsAssetLoaded();
		if (IsLoaded==0)
		pAnimationSet->StreamCAF(nGlobalID,0);
		}*/

		globalAnimHeaderCAF.AllocateAnimSelectProps();
	}

	if (m_arrAnimations[localAnimID].m_nAssetType==LMG_File)
	{
		uint32 num = g_AnimationManager.m_arrGlobalLMG.size();
		assert(int32(num)>globalAnimID);
		GlobalAnimationHeaderLMG& globalAnimHeaderLMG = g_AnimationManager.m_arrGlobalLMG[globalAnimID];
		if (globalAnimHeaderLMG.m_pSelectionProperties != NULL)
			return true;
		if (!globalAnimHeaderLMG.IsAssetCreated())
			return false;

		globalAnimHeaderLMG.AllocateAnimSelectProps();
	}




#define BLENDCODE(code)		(*(uint32*)code)


	if (m_arrAnimations[localAnimID].m_nAssetType==LMG_File)
	{
		GlobalAnimationHeaderLMG& globalAnimHeaderLMG = g_AnimationManager.m_arrGlobalLMG[globalAnimID];

		// Recurse sub animations first
		uint32 numSubAnims = globalAnimHeaderLMG.m_arrBSAnimations.size();
		for (uint32 i = 0; i < numSubAnims; ++i)
		{
			const char* szModelAnimNameSub = globalAnimHeaderLMG.m_arrBSAnimations[i].m_strAnimName; //0
			int32 localAnimIDSub = GetAnimIDByName(szModelAnimNameSub);
			assert(localAnimIDSub >= 0);
			if (localAnimIDSub < 0)
				return false;
			if (!ComputeSelectionProperties(localAnimIDSub))
				return false;
		}

		{
			assert(globalAnimHeaderLMG.m_pSelectionProperties!=0);
			globalAnimHeaderLMG.m_pSelectionProperties->m_fStartTravelSpeedMin = 100.0f;
			globalAnimHeaderLMG.m_pSelectionProperties->m_fEndTravelSpeedMin = 100.0f;
		}

		// Then use their limits to expand LMG limits.
		for (uint32 i = 0; i < numSubAnims; ++i)
		{
			const char* szModelAnimNameSub = globalAnimHeaderLMG.m_arrBSAnimations[i].m_strAnimName; //0
			int32 localAnimIDSub = GetAnimIDByName(szModelAnimNameSub);
			assert(localAnimIDSub >= 0);
			int32 globalAnimIDSub = m_arrAnimations[localAnimIDSub].m_nGlobalAnimId;
			assert(globalAnimIDSub >= 0);
			GlobalAnimationHeaderCAF& globalAnimHeaderSub = g_AnimationManager.m_arrGlobalCAF[globalAnimIDSub];
			globalAnimHeaderLMG.m_pSelectionProperties->expand(globalAnimHeaderSub.m_pSelectionProperties);
		}

		SAnimationSelectionProperties& proceduralProperties = *globalAnimHeaderLMG.m_pSelectionProperties;

		//#ifdef _DEBUG
		proceduralProperties.DebugCapsCode = globalAnimHeaderLMG.m_nSelectionCapsCode;
		//#endif

		if (globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("IROT"))
		{
			proceduralProperties.m_fStartTravelAngleMin = -180.0f;
			proceduralProperties.m_fStartTravelAngleMax = +180.0f;
			proceduralProperties.m_fEndTravelAngleMin = -180.0f;
			proceduralProperties.m_fEndTravelAngleMax = +180.0f;
			proceduralProperties.m_fEndBodyAngleMin = -180.0f;
			proceduralProperties.m_fEndBodyAngleMax = +180.0f;
			proceduralProperties.m_fTravelAngleChangeMin = -180.0f;
			proceduralProperties.m_fTravelAngleChangeMax = +180.0f;
			proceduralProperties.m_fTravelDistanceMin = 0.0f;
			proceduralProperties.m_fTravelDistanceMax = 0.0f;
			proceduralProperties.m_fStartTravelSpeedMin = 0.0f;
			proceduralProperties.m_fStartTravelSpeedMax = 0.0f;
			proceduralProperties.m_fEndTravelSpeedMin = 0.0f;
			proceduralProperties.m_fEndTravelSpeedMax = 0.0f;
			proceduralProperties.m_fEndTravelToBodyAngleMin = -180.0f;
			proceduralProperties.m_fEndTravelToBodyAngleMax = +180.0f;
			proceduralProperties.m_fUrgencyMin = 0.0f;
			proceduralProperties.m_fUrgencyMax = 0.0f;
			proceduralProperties.m_bLocomotion = true;
			proceduralProperties.m_fEndBodyAngleThreshold = 70.0f; // NOTE: This should always be less than the clamping angle, or the turn will never get a chance.
		}

		/*
		if (globalAnimHeader.m_nSelectionCapsCode == BLENDCODE("PROT"))
		{
		proceduralProperties.m_fStartTravelAngleMin = -180.0f;
		proceduralProperties.m_fStartTravelAngleMax = +180.0f;
		proceduralProperties.m_fEndTravelAngleMin = -180.0f;
		proceduralProperties.m_fEndTravelAngleMax = +180.0f;
		proceduralProperties.m_fEndBodyAngleMin = -180.0f;
		proceduralProperties.m_fEndBodyAngleMax = +180.0f;
		proceduralProperties.m_fTravelAngleChangeMin = -180.0f;
		proceduralProperties.m_fTravelAngleChangeMax = +180.0f;
		proceduralProperties.m_fTravelDistanceMin = 0.0f;
		proceduralProperties.m_fTravelDistanceMax = 0.0f;
		proceduralProperties.m_fStartTravelSpeedMin = 0.0f;
		proceduralProperties.m_fStartTravelSpeedMax = 0.0f;
		proceduralProperties.m_fEndTravelSpeedMin = 0.0f;
		proceduralProperties.m_fEndTravelSpeedMax = 0.0f;
		proceduralProperties.m_fEndTravelToBodyAngleMin = -180.0f;
		proceduralProperties.m_fEndTravelToBodyAngleMax = +180.0f;
		proceduralProperties.m_fUrgencyMin = 0.0f;
		proceduralProperties.m_fUrgencyMax = 0.0f;
		proceduralProperties.m_bLocomotion = true;
		proceduralProperties.m_fEndBodyAngleThreshold = 70.0f; // NOTE: This should always be less than the clamping angle, or the turn will never get a chance.
		}
		*/

		if (globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("ISTP"))
		{
			proceduralProperties.m_fStartTravelAngleMin = -180.0f;
			proceduralProperties.m_fStartTravelAngleMax = +180.0f;
			proceduralProperties.m_fEndTravelAngleMin = -180.0f;
			proceduralProperties.m_fEndTravelAngleMax = +180.0f;
			proceduralProperties.m_fTravelDistanceMin = 0.3f;
			proceduralProperties.m_fTravelDistanceMax = 1.0f;
			proceduralProperties.m_fEndBodyAngleMin = 0.0f;
			proceduralProperties.m_fEndBodyAngleMax = 0.0f;
			proceduralProperties.m_fTravelAngleChangeMin = -180.0f;
			proceduralProperties.m_fTravelAngleChangeMax = +180.0f;
			proceduralProperties.m_fStartTravelSpeedMin = 0.0f;
			proceduralProperties.m_fStartTravelSpeedMax = 0.5f;
			proceduralProperties.m_fEndTravelSpeedMin = 0.0f;
			proceduralProperties.m_fEndTravelSpeedMax = 0.5f;
			proceduralProperties.m_fEndTravelToBodyAngleMin = -180.0f;
			proceduralProperties.m_fEndTravelToBodyAngleMax = +180.0f;
			proceduralProperties.m_fUrgencyMin = 0.0f;
			proceduralProperties.m_fUrgencyMax = 0.0f;
			proceduralProperties.m_bLocomotion = true;
			proceduralProperties.m_fTravelDistanceThreshold = 0.1f;
		}

		if (globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("I2M_") ||		globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("I2W_"))
		{
			proceduralProperties.m_fStartTravelAngleMin = -180.0f;
			proceduralProperties.m_fStartTravelAngleMax = +180.0f;
			proceduralProperties.m_fEndTravelAngleMin = -180.0f;
			proceduralProperties.m_fEndTravelAngleMax = +180.0f;
			proceduralProperties.m_fEndBodyAngleMin = -180.0f;
			proceduralProperties.m_fEndBodyAngleMax = +180.0f;
			proceduralProperties.m_fTravelAngleChangeMin = -30.0f;
			proceduralProperties.m_fTravelAngleChangeMax = +30.0f;
			proceduralProperties.m_fDurationMin = 1.0f;
			proceduralProperties.m_fDurationMax = 10.0f;
			proceduralProperties.m_fTravelDistanceMin = 1.6f;
			proceduralProperties.m_fTravelDistanceMax = 10.0f;
			proceduralProperties.m_fStartTravelSpeedMin = 0.0f;
			proceduralProperties.m_fStartTravelSpeedMax = 0.8f;
			if (globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("I2W_"))
			{
				proceduralProperties.m_fStartTravelSpeedMax = 0.1f;
				proceduralProperties.m_fUrgencyMin = 0.2f;
				proceduralProperties.m_fUrgencyMax = 0.9f;
				proceduralProperties.m_fTravelDistanceMin = 0.4f;
				proceduralProperties.m_fTravelDistanceMax = 5.0f;
			}
			else
			{
				proceduralProperties.m_fUrgencyMin = 1.0f;
				proceduralProperties.m_fUrgencyMax = 3.0f;
			}
			//proceduralProperties.m_fEndTravelSpeedMin *= Console::GetInst().ca_travelSpeedScaleMin;
			//proceduralProperties.m_fEndTravelSpeedMax *= Console::GetInst().ca_travelSpeedScaleMax;
			proceduralProperties.m_fEndTravelToBodyAngleMin = -30.0f;
			proceduralProperties.m_fEndTravelToBodyAngleMax = +30.0f;
			proceduralProperties.m_bPredicted = true;
			proceduralProperties.m_bGuarded = true;
			proceduralProperties.m_bComplexBodyTurning = false; // These two are not supported anymore and are instead specified in the AG template (optimization).
			proceduralProperties.m_bComplexTravelPath = false;
			proceduralProperties.m_bLocomotion = true;
			//proceduralProperties.m_fTravelDistanceThreshold = 1.0f;
		}

		if (globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("M2I_") ||	globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("W2I_"))
		{
			/*
			proceduralProperties.m_fStartTravelAngleMin = -180.0f;
			proceduralProperties.m_fStartTravelAngleMax = +180.0f;
			proceduralProperties.m_fEndTravelAngleMin = -180.0f;
			proceduralProperties.m_fEndTravelAngleMax = +180.0f;
			proceduralProperties.m_fEndBodyAngleMin = -180.0f;
			proceduralProperties.m_fEndBodyAngleMax = +180.0f;
			*/
			proceduralProperties.m_fStartTravelAngleMin = -5.0f;
			proceduralProperties.m_fStartTravelAngleMax = +5.0f;
			proceduralProperties.m_fEndTravelAngleMin = -5.0f;
			proceduralProperties.m_fEndTravelAngleMax = +5.0f;
			proceduralProperties.m_fEndBodyAngleMin = -5.0f;
			proceduralProperties.m_fEndBodyAngleMax = +5.0f;
			proceduralProperties.m_fTravelAngleChangeMin = -5.0f;
			proceduralProperties.m_fTravelAngleChangeMax = 5.0f;
			proceduralProperties.m_fDurationMin = 1.0f;
			proceduralProperties.m_fDurationMax = 1.0f;
			proceduralProperties.m_fTravelDistanceMin = proceduralProperties.m_fTravelDistanceMax * 1.1f;
			proceduralProperties.m_fTravelDistanceMax = proceduralProperties.m_fTravelDistanceMax * 1.2f;
			//proceduralProperties.m_fStartTravelSpeedMin *= Console::GetInst().ca_travelSpeedScaleMin;
			proceduralProperties.m_fStartTravelSpeedMax *= Console::GetInst().ca_travelSpeedScaleMax * 4.0f;
			proceduralProperties.m_fEndTravelSpeedMin = 0.0f;
			proceduralProperties.m_fEndTravelSpeedMax = 0.5f;

			if (globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("W2I_"))
			{
				proceduralProperties.m_fUrgencyMin = 0.2f;
				proceduralProperties.m_fUrgencyMax = 0.9f;
			}
			else
			{
				proceduralProperties.m_fUrgencyMin = 1.0f;
				proceduralProperties.m_fUrgencyMax = 3.0f;
			}

			proceduralProperties.m_fEndTravelToBodyAngleMin = -5.0f;
			proceduralProperties.m_fEndTravelToBodyAngleMax = +5.0f;
			proceduralProperties.m_bPredicted = true;
			proceduralProperties.m_bGuarded = true;
			proceduralProperties.m_bLocomotion = true;
			//proceduralProperties.m_fTravelDistanceThreshold = 1.0f;
		}

		// WALKS
		if (globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("WALK"))
		{
			proceduralProperties.m_fDurationMin = 0.0f;
			proceduralProperties.m_fDurationMax = 10.0f;
			proceduralProperties.m_fTravelDistanceMin = 0.0f;
			proceduralProperties.m_fTravelDistanceMax = 10.0f;
			//proceduralProperties.m_fStartTravelSpeedMin = 1.0f;
			proceduralProperties.m_fStartTravelSpeedMin *= Console::GetInst().ca_travelSpeedScaleMin * 0.0f;
			proceduralProperties.m_fStartTravelSpeedMax *= Console::GetInst().ca_travelSpeedScaleMax;
			proceduralProperties.m_fEndTravelSpeedMin *= Console::GetInst().ca_travelSpeedScaleMin * 0.0f;
			proceduralProperties.m_fEndTravelSpeedMax *= Console::GetInst().ca_travelSpeedScaleMax;
			proceduralProperties.m_fStartTravelAngleMin = -180.0f;
			proceduralProperties.m_fStartTravelAngleMax = +180.0f;
			proceduralProperties.m_fEndTravelAngleMin = -180.0f;
			proceduralProperties.m_fEndTravelAngleMax = +180.0f;
			proceduralProperties.m_fEndBodyAngleMin = -180.0f;
			proceduralProperties.m_fEndBodyAngleMax = +180.0f;
			proceduralProperties.m_fTravelAngleChangeMin = -180.0f;
			proceduralProperties.m_fTravelAngleChangeMax = +180.0f;
			proceduralProperties.m_fEndTravelToBodyAngleMin = -180.0f;
			proceduralProperties.m_fEndTravelToBodyAngleMax = +180.0f;
			proceduralProperties.m_fUrgencyMin = 0.2f;
			proceduralProperties.m_fUrgencyMax = 0.5f;
			proceduralProperties.m_bLocomotion = true;
		}

		// RUN+SPRINT
		if (globalAnimHeaderLMG.m_nSelectionCapsCode == BLENDCODE("RUN_"))
		{
			f32 fScale = 1.0f; 
			proceduralProperties.m_fDurationMin = 0.0f;
			proceduralProperties.m_fDurationMax = 10.0f;
			proceduralProperties.m_fTravelDistanceMin = 0.0f;
			proceduralProperties.m_fTravelDistanceMax = 10.0f;
			//proceduralProperties.m_fStartTravelSpeedMin = 1.0f;
			proceduralProperties.m_fStartTravelSpeedMin *= Console::GetInst().ca_travelSpeedScaleMin * 0.0f;
			proceduralProperties.m_fStartTravelSpeedMax *= Console::GetInst().ca_travelSpeedScaleMax;
			proceduralProperties.m_fEndTravelSpeedMin *= Console::GetInst().ca_travelSpeedScaleMin * 0.0f;
			//proceduralProperties.m_fEndTravelSpeedMin = max(proceduralProperties.m_fStartTravelSpeedMin, proceduralProperties.m_fEndTravelSpeedMin);
			proceduralProperties.m_fEndTravelSpeedMax *= Console::GetInst().ca_travelSpeedScaleMax;
			proceduralProperties.m_fStartTravelAngleMin = -180.0f*fScale;
			proceduralProperties.m_fStartTravelAngleMax = +180.0f*fScale;
			proceduralProperties.m_fEndTravelAngleMin = -180.0f*fScale;
			proceduralProperties.m_fEndTravelAngleMax = +180.0f*fScale;
			proceduralProperties.m_fEndBodyAngleMin = -180.0f*fScale;
			proceduralProperties.m_fEndBodyAngleMax = +180.0f*fScale;
			proceduralProperties.m_fTravelAngleChangeMin = -180.0f*fScale;
			proceduralProperties.m_fTravelAngleChangeMax = +180.0f*fScale;
			proceduralProperties.m_fEndTravelToBodyAngleMin = -180.0f*fScale;
			proceduralProperties.m_fEndTravelToBodyAngleMax = +180.0f*fScale;
			proceduralProperties.m_fUrgencyMin = 0.9f;
			proceduralProperties.m_fUrgencyMax = 2.0f;
			proceduralProperties.m_bLocomotion = true;
		}

		//globalAnimHeader.m_selectionProperties = proceduralProperties;
	}
	else 
	{
		GlobalAnimationHeaderCAF& globalAnimHeaderCAF = g_AnimationManager.m_arrGlobalCAF[globalAnimID];

		if (globalAnimHeaderCAF.m_pSelectionProperties != NULL)
		{
			// Measure the trajectory of the anim to figure out all the properties.
			SAnimationSelectionProperties& props = *globalAnimHeaderCAF.m_pSelectionProperties;

			props.m_fDurationMin = globalAnimHeaderCAF.m_fEndSec - globalAnimHeaderCAF.m_fStartSec;

			int jointIndex = 0;
			const CModelJoint& modelJoint = m_pModel->m_ModelSkeleton.m_arrModelJoints[jointIndex];


			GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[globalAnimID];
			if (rGAH.m_nControllers==0)
			{
				if (rGAH.m_FilePathDBACRC32)
				{
					size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
					for (uint32 d=0; d<numDBA_Files; d++)
					{
						CGlobalHeaderDBA& pGlobalHeaderDBA = *g_AnimationManager.m_arrGlobalHeaderDBA[d];
						if (rGAH.m_FilePathDBACRC32!=pGlobalHeaderDBA.m_FilePathDBACRC32)
							continue;

						if (pGlobalHeaderDBA.m_pDatabaseInfo==0)
						{
							const char* pName  = pGlobalHeaderDBA.m_strFilePathDBA;
							pGlobalHeaderDBA.StreamDatabaseDBA();
						}

						if (pGlobalHeaderDBA.m_pDatabaseInfo)
						{
							const CCommonSkinningInfo* pSkinningInfo = pGlobalHeaderDBA.GetSkinningInfoDBA(rGAH.m_FilePath);
							uint32 numController = pSkinningInfo->m_pControllers.size();
							if (numController!=rGAH.m_nControllers2)
								CryFatalError("Controller mismatch");

							for(uint32 i=0; i<numController; i++ )
								rGAH.m_arrController[i] = pSkinningInfo->m_pControllers[i];
							rGAH.m_nControllers = numController;
							std::sort(rGAH.m_arrController,	rGAH.m_arrController + numController, AnimCtrlSortPred()	);
							rGAH.InitControllerLookup();
							rGAH.OnAssetCreated();
							rGAH.OnAssetLoaded();
							rGAH.ClearAssetRequested();
						}
						assert(pGlobalHeaderDBA.m_pDatabaseInfo);
						break;
					}
				}
			}

			IController* pController = g_AnimationManager.m_arrGlobalCAF[globalAnimID].GetControllerByJointCRC32(modelJoint.m_nJointCRC32);
			if (pController == NULL)
				return false;

			static float samplesPerSecond = 20.0f;
			int sampleCount = max(2, (int)(samplesPerSecond * props.m_fDurationMin));
			float sampleDuration = props.m_fDurationMin / (float)sampleCount;

			// These allocating should maybe be moved outside the loop and reused for all assets, allocated at the maximum size, or implemented as chunks.
			Vec3* locomotionLocatorSamplesPos = new Vec3[sampleCount];
			Quat* locomotionLocatorSamplesOri = new Quat[sampleCount];
			Vec3* locomotionLocatorSamplesMovement = new Vec3[sampleCount];
			Vec3* locomotionLocatorSamplesMovementSmoothed = new Vec3[sampleCount];
			Vec3* locomotionLocatorSamplesBodyDirSmoothed = new Vec3[sampleCount];

			// Evaluate translation & orientation of joint 0 of the animation asset.
			for (int i = 0; i < sampleCount; ++i)
			{
				float t = (float)i / (float)(sampleCount - 1);
				pController->GetP( globalAnimHeaderCAF.NTime2KTime(t), locomotionLocatorSamplesPos[i]);
				pController->GetO( globalAnimHeaderCAF.NTime2KTime(t), locomotionLocatorSamplesOri[i]);
			}

			for (int i = 0; i < sampleCount; ++i)
			{
				int i0 = max(i - 1, 0);
				int i1 = min(i + 1, sampleCount - 1);
				locomotionLocatorSamplesMovement[i] = locomotionLocatorSamplesPos[i1] - locomotionLocatorSamplesPos[i0];
				float distance = (float)(i1 - i0);
				if (distance > 0.0f)
					locomotionLocatorSamplesMovement[i] /= distance;
			}

			static int windowSampleCount = 10;
			for (int i = 0; i < sampleCount; ++i)
			{
				locomotionLocatorSamplesBodyDirSmoothed[i] = ZERO;
				locomotionLocatorSamplesMovementSmoothed[i] = ZERO;
				float weightSum = 0.0f;
				for (int j = 0; j < windowSampleCount; ++j)
				{
					int k = CLAMP(i - windowSampleCount/2 + j, 0, sampleCount-1);
					float weight = 1.0f - CLAMP((float)(k - i) / (float)windowSampleCount, 0.0f, 1.0f);
					weightSum += weight;
					locomotionLocatorSamplesMovementSmoothed[i] += locomotionLocatorSamplesMovement[k] * weight;

					Vec3 locomotionLocatorBodyDir = locomotionLocatorSamplesOri[i] * FORWARD_DIRECTION;
					locomotionLocatorSamplesBodyDirSmoothed[i] += locomotionLocatorBodyDir * weight;
				}
				if (weightSum > 0.0f)
				{
					locomotionLocatorSamplesMovementSmoothed[i] /= weightSum;
					locomotionLocatorSamplesBodyDirSmoothed[i] /= weightSum;
				}
			}

			float sampleDurationInv = (sampleDuration > 0.0f) ? 1.0f / sampleDuration : 0.0f;
			props.m_fTravelDistanceMin = (locomotionLocatorSamplesPos[sampleCount-1] - locomotionLocatorSamplesPos[0]).GetLength();
			props.m_fStartTravelSpeedMin = locomotionLocatorSamplesMovementSmoothed[0].GetLength() * sampleDurationInv;
			props.m_fEndTravelSpeedMin = locomotionLocatorSamplesMovementSmoothed[sampleCount-1].GetLength() * sampleDurationInv;
			props.m_fStartTravelAngleMin = DEG2RAD(cry_atan2f(-locomotionLocatorSamplesMovementSmoothed[0].x, locomotionLocatorSamplesMovementSmoothed[0].y));
			props.m_fEndTravelAngleMin = DEG2RAD(cry_atan2f(-locomotionLocatorSamplesMovementSmoothed[sampleCount-1].x, locomotionLocatorSamplesMovementSmoothed[sampleCount-1].y));
			float fStartBodyAngle = DEG2RAD(cry_atan2f(-locomotionLocatorSamplesBodyDirSmoothed[0].x, locomotionLocatorSamplesBodyDirSmoothed[0].y));
			float fEndBodyAngle = DEG2RAD(cry_atan2f(-locomotionLocatorSamplesBodyDirSmoothed[sampleCount-1].x, locomotionLocatorSamplesBodyDirSmoothed[sampleCount-1].y));
			props.m_fEndBodyAngleMin = (fEndBodyAngle - fStartBodyAngle);
			if (props.m_fEndBodyAngleMin < -180.0f)
				props.m_fEndBodyAngleMin += 360.0f;
			if (props.m_fEndBodyAngleMin > 180.0f)
				props.m_fEndBodyAngleMin -= 360.0f;

			if (abs(props.m_fStartTravelAngleMin) < 0.0001f) props.m_fStartTravelAngleMin = 0.0f;
			if (abs(props.m_fEndTravelAngleMin) < 0.0001f) props.m_fEndTravelAngleMin = 0.0f;
			if (abs(props.m_fEndBodyAngleMin) < 0.0001f) props.m_fEndBodyAngleMin = 0.0f;

			props.m_fTravelAngleChangeMin = props.m_fEndTravelAngleMin - props.m_fStartTravelAngleMin;

			props.m_fDurationMax					= props.m_fDurationMin;
			props.m_fStartTravelSpeedMax	= props.m_fStartTravelSpeedMin;
			props.m_fEndTravelSpeedMax		=	props.m_fEndTravelSpeedMin;
			props.m_fStartTravelAngleMax	= props.m_fStartTravelAngleMin;
			props.m_fEndTravelAngleMax		=	props.m_fEndTravelAngleMin;
			props.m_fEndBodyAngleMin			=	props.m_fEndBodyAngleMax;
			props.m_fTravelDistanceMax		=	props.m_fTravelDistanceMin;
			props.m_fTravelAngleChangeMax = props.m_fTravelAngleChangeMin;

			delete[] locomotionLocatorSamplesPos;
			delete[] locomotionLocatorSamplesOri;
			delete[] locomotionLocatorSamplesMovement;
			delete[] locomotionLocatorSamplesMovementSmoothed;
			delete[] locomotionLocatorSamplesBodyDirSmoothed;
		}
	}

	return true;
}



void CAnimationSet::ComputeSelectionProperties()
{
	/*
	int32 animationCount = m_arrAnimations.size();
	for (int32 localAnimID = 0; localAnimID < animationCount; localAnimID++)
	{
	bool success = ComputeSelectionProperties(localAnimID);
	//assert(success);
	}
	*/
}


//----------------------------------------------------------------------------------
//----      check if all animation-assets in a locomotion group are valid       ----
//----------------------------------------------------------------------------------
void CAnimationSet::VerifyLMGs()
{
	uint32 numAnimNames = m_arrAnimations.size();
	for (uint32 i=0; i<numAnimNames; i++)
	{
		VerifyLocomotionGroup(i);
	}
}

LmgAnimationStatus CAnimationSet::IsAnimationValidForLMG(const char* lmgBlendCode, const char* szAnimationName)
{
	uint32 nBC = *(uint32*)lmgBlendCode;

	return IsAnimationValidForLMG(nBC, szAnimationName);
}

LmgAnimationStatus CAnimationSet::IsAnimationValidForLMG(uint32 lmgBlendCode, const char* szAnimationName)
{
	if (!LMG::IsValidBlendCode(lmgBlendCode))
	{
		return LMGAS_INVALID_BLEND_CODE;
	}

	int32 id = GetAnimIDByName(szAnimationName);
	if (id < 0)
	{
		return LMGAS_ANIMATION_NOT_IN_CAL_FILE;
	}

	uint32 GlobalAnimationID = m_arrAnimations[id].m_nGlobalAnimId;
	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];

	if (rGlobalAnimHeader.IsAssetNotFound() || rGlobalAnimHeader.m_fStartSec<0 || rGlobalAnimHeader.m_fEndSec<0)
	{
		return LMGAS_ASSET_DOESNT_EXIST;
	}

	if (LMG::IsStrafingLMGCode(lmgBlendCode))
	{
		f32 speed = rGlobalAnimHeader.m_fSpeed;
		if (speed<0.1f)
		{
			return LMGAS_ANIMATION_HAS_NO_SPEED;
		}		
	}

	return LMGAS_OK;
}

void CAnimationSet::VerifyLocomotionGroup(int animationID)
{
	uint32 GlobalAnimationID			= m_arrAnimations[animationID].m_nGlobalAnimId;
	bool lmg = (m_arrAnimations[animationID].m_nAssetType==LMG_File);
	if (!lmg)
	{
		return;
	}

	GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[GlobalAnimationID];

	if (!rGlobalAnimHeader.IsAssetCreated())
	{
		return;
	}

	//its a locomotion group
	uint32 numLMG = rGlobalAnimHeader.m_arrBSAnimations.size();
	uint32 LMG_OK=numLMG;
	uint32 blendCodeLmg = rGlobalAnimHeader.m_nBlendCodeLMG;
	for (uint32 g=0; g<numLMG; g++)
	{
		const char* aname = rGlobalAnimHeader.m_arrBSAnimations[g].m_strAnimName;
		LmgAnimationStatus lmgAnimationStatus = IsAnimationValidForLMG( blendCodeLmg, aname );
		if ( lmgAnimationStatus != LMGAS_OK )
		{
			LMG_OK = 0;

			if ( lmgAnimationStatus == LMGAS_ANIMATION_NOT_IN_CAL_FILE )
			{
				AnimFileWarning(m_pModel->GetModelFilePath(),"locomotion group '%s' is invalid! The animation '%s' is not in the CAL-file.", m_arrAnimations[animationID].GetAnimName(),aname);
			}
			else if ( lmgAnimationStatus == LMGAS_ASSET_DOESNT_EXIST )
			{
				AnimFileWarning(m_pModel->GetModelFilePath(),"locomotion group '%s' is invalid! The the asset for animation '%s' does not exist.", m_arrAnimations[animationID].GetAnimName(),aname);
			}			
		}
	}

	if (LMG_OK)
	{
		rGlobalAnimHeader.OnAssetLMGValid();
		LMG::ExtractParameters(this, rGlobalAnimHeader);

	}
	uint32 numAssets = rGlobalAnimHeader.m_arrBSAnimations.size();
	for (uint32 i=0; i<numAssets; i++)
	{
		const char* name = rGlobalAnimHeader.m_arrBSAnimations[i].m_strAnimName; //0
		int32 globalID = GetGlobalIDByName(name);
		const bool bValidGlobalId = (globalID >= 0);
		ANIM_ASSET_CHECK_TRACE(bValidGlobalId, ("Couldn't find a valid asset for %s", name));
		if (!bValidGlobalId)
		{
			continue;
		}

		uint32 OnDemand=g_AnimationManager.m_arrGlobalCAF[globalID].IsAssetOnDemand();
		if (OnDemand)
		{
			if ( g_AnimationManager.m_arrGlobalCAF[globalID].IsAssetLoaded() )
						g_AnimationManager.UnloadAnimationCAF(globalID);
		}
	}
}


//---------------------------------------------------------------------------------------
// [artemk]: Helper functions for computing animation derived parameters
//---------------------------------------------------------------------------------------
IController* CAnimationSet::GAH_GetRootController(GlobalAnimationHeaderCAF& rGAH, uint32 globalID, const CCharacterModel* pModel, const char* pAnimName)
{
	const CModelJoint* pRootJoint	= &pModel->m_ModelSkeleton.m_arrModelJoints[0];
	IController* pController = rGAH.GetControllerByJointCRC32(pRootJoint->m_nJointCRC32);
	if (pController==0)
		rGAH.LoadControllersCAF();
	pController	= rGAH.GetControllerByJointCRC32(pRootJoint[0].m_nJointCRC32);
	if (pController==0)
	{
		const char* pModelName = m_pModel->GetFilePath();
		g_pISystem->Warning( VALIDATOR_MODULE_ANIMATION,VALIDATOR_WARNING,	VALIDATOR_FLAG_FILE, pModelName, "LMG is invalid. Locomotion-asset '%s' not in memory ",pAnimName );

		rGAH.m_nFlags =  rGAH.m_nFlags & (CA_ASSET_LMG_VALID^-1);
		return 0;
	}

	return pController;
}

#define NumKeys (50)

static void ComputeLocatorSpeedPMG(const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, const int GAID, IController* pRootCtl, const uint32 animInd)
{
	assert(pRootCtl);

	f32 fDuration = rGAH.m_fEndSec-rGAH.m_fStartSec; 	//assert(fDuration>0.0f);
	f32 fTotalKeys = (fDuration * 30);
	uint32 totalKeys = uint32( fDuration * 30 );
	if(totalKeys == 0) // at least one frame
		totalKeys = 1;

	//------------------------------------------------------------------------------
	// If user specified PMG parameters, we should change the locator parameters to
	// user specified ones

	if( !pmgHeader.m_userParam.empty() )
	{
		uint32 iSize = (uint32)pmgHeader.m_userParam[animInd].size();
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "MoveSpeed")
			{
				rGAH.m_arrLocoMoveSpeedPMG.resize(totalKeys, pmgHeader.m_userParam[animInd][i].val);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------
	// Normal calculation

	rGAH.m_arrLocoMoveSpeedPMG.resize(totalKeys);  ///meters per second

	for (uint32 k=1; k<totalKeys; ++k)
	{
		f32 key=f32(k);
		QuatT prev;	
		f32 ts = f32(key-1)/f32(totalKeys);
		pRootCtl->GetOP( rGAH.NTime2KTime(ts), prev.q, prev.t );
		QuatT now;
		f32 te = f32(key)/f32(totalKeys);
		pRootCtl->GetOP( rGAH.NTime2KTime(te), now.q, now.t );

		const Vec3r pDir = prev.q.GetColumn1();
		const Vec3r cDir = now.q.GetColumn1();
		f32 turnAngle0 = f32( Ang3r::CreateRadZ(pDir, cDir) )*30.0f;

		Vec3 dist0=((prev.t-now.t)*30.0f);
		Vec3 velocity = dist0*now.q;  ///meters per second
		rGAH.m_arrLocoMoveSpeedPMG[k] = velocity.GetLength();  ///meters per second
		if (rGAH.m_arrLocoMoveSpeedPMG[k]<0.0001f)
			rGAH.m_arrLocoMoveSpeedPMG[k]=0;
	}

	if (totalKeys>1)
		rGAH.m_arrLocoMoveSpeedPMG[0]=rGAH.m_arrLocoMoveSpeedPMG[1];  ///meters per second

}

static void ComputeLocatorStrifeAnglePMG(const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, const uint32 animInd)
{
	assert(pRootCtl);

	f32 fDuration = rGAH.m_fEndSec-rGAH.m_fStartSec; 	//assert(fDuration>0.0f);
	f32 fTotalKeys = (fDuration * 30);
	uint32 totalKeys = uint32( fDuration * 30 );
	if(totalKeys == 0) // at least one frame
		totalKeys = 1;

	//------------------------------------------------------------------------------
	// If user specified PMG parameters, we should change the locator parameters to
	// user specified ones

	if( !pmgHeader.m_userParam.empty() )
	{
		uint32 iSize = (uint32)pmgHeader.m_userParam[animInd].size();
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "TravelDir")
			{
				rGAH.m_arrLocoHorizAnglePMG.resize(totalKeys, pmgHeader.m_userParam[animInd][i].val);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------
	// Normal calculation

	rGAH.m_arrLocoHorizAnglePMG.resize(totalKeys);


	for (uint32 k=1; k<totalKeys; ++k)
	{
		f32 key=f32(k);
		QuatT prev;	
		f32 ts = f32(key-1)/f32(totalKeys);
		pRootCtl->GetOP( rGAH.NTime2KTime(ts), prev.q, prev.t );
		QuatT now;
		f32 te = f32(key)/f32(totalKeys);
		pRootCtl->GetOP( rGAH.NTime2KTime(te), now.q, now.t );

		const Vec3r pDir = prev.q.GetColumn1();
		const Vec3r cDir = now.q.GetColumn1();
		f32 turnAngle0 = f32( Ang3r::CreateRadZ(pDir, cDir) )*30.0f;


		Vec3 dist0=((prev.t-now.t)*30.0f);

		Vec3 velocity = dist0*now.q;  ///meters per second
		rGAH.m_arrLocoHorizAnglePMG[k] = Ang3::CreateRadZ(Vec3(0,1,0),-velocity);
		if (rGAH.m_arrLocoMoveSpeedPMG[k]<0.0001f)	
			rGAH.m_arrLocoHorizAnglePMG[k]=0;
	}

	if (totalKeys>1)
	{
		rGAH.m_arrLocoHorizAnglePMG[0]=rGAH.m_arrLocoHorizAnglePMG[1];
	}
}


static void ComputeLocatorTurnSpeedPMG(const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl,const uint32 animInd)
{
	assert(pRootCtl);

	//------------------------------------------------------------------------------
	// Turn angle per second
	f32 fDuration = rGAH.m_fEndSec-rGAH.m_fStartSec; 	//assert(fDuration>0.0f);
	f32 fTotalKeys = (fDuration * 30);
	uint32 totalKeys = uint32( fDuration * 30 );
	if(totalKeys == 0) // at least one frame
		totalKeys = 1;

	//------------------------------------------------------------------------------
	// If user specified PMG parameters, we should change the locator parameters to
	// user specified ones

	if( !pmgHeader.m_userParam.empty() )
	{
		uint32 iSize = (uint32)pmgHeader.m_userParam[animInd].size();
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "TurnSpeed")
			{
				rGAH.m_arrLocoTurnSpeedPMG.resize(totalKeys, pmgHeader.m_userParam[animInd][i].val);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------
	// Normal calculation

	rGAH.m_arrLocoTurnSpeedPMG.resize(totalKeys);
	for (uint32 k=1; k<totalKeys; ++k)
	{
		f32 key=f32(k);
		QuatT prev;	
		f32 ts = f32(key-1)/f32(totalKeys);
		pRootCtl->GetOP( rGAH.NTime2KTime(ts), prev.q, prev.t );
		QuatT now;
		f32 te = f32(key)/f32(totalKeys);
		pRootCtl->GetOP( rGAH.NTime2KTime(te), now.q, now.t );
		//		QuatT next;
		//		f32 tn = f32(key+1)/f32(totalKeys);
		//		pRootCtl->GetOP( GAID, tn, next.q, next.t );

		const Vec3r pDir = prev.q.GetColumn1();
		const Vec3r cDir = now.q.GetColumn1();
		//	const Vec3r nDir = next.q.GetColumn1();
		f32 turnAngle0 = f32( Ang3r::CreateRadZ(pDir, cDir) )*30.0f;
		//	f32 turnAngle1 = f32( Ang3r::CreateRadZ(cDir, nDir) )*30.0f;

		//	rGAH.m_arrLocoTurnSpeedPMG[k] = (turnAngle0+turnAngle1)*0.5f; 
		rGAH.m_arrLocoTurnSpeedPMG[k] = turnAngle0; 
	}

	if (totalKeys>1)
		rGAH.m_arrLocoTurnSpeedPMG[0]=rGAH.m_arrLocoTurnSpeedPMG[1];
}

static void ComputeLocatorVerticalMovPMG(const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl, const uint32 animInd)
{
	assert(pRootCtl);

	//------------------------------------------------------------------------------
	// Turn angle per second
	f32 fDuration = rGAH.m_fEndSec-rGAH.m_fStartSec; 	//assert(fDuration>0.0f);
	f32 fTotalKeys = (fDuration * 30);
	uint32 totalKeys = uint32( fDuration * 30 );
	if(totalKeys == 0) // at least one frame
		totalKeys = 1;

	//------------------------------------------------------------------------------
	// If user specified PMG parameters, we should change the locator parameters to
	// user specified ones

	if( !pmgHeader.m_userParam.empty() )
	{
		uint32 iSize = (uint32)pmgHeader.m_userParam[animInd].size();
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "Slope")
			{
				rGAH.m_arrLocoVertAnglePMG.resize(totalKeys, pmgHeader.m_userParam[animInd][i].val);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------
	// Normal calculation

	rGAH.m_arrLocoVertAnglePMG.resize(totalKeys);
	for (uint32 k=1; k<totalKeys; ++k)
	{
		f32 key=f32(k);
		f32 ts = f32(key-1)/f32(totalKeys);
		Vec3 TravelDir0;
		pRootCtl->GetP( rGAH.NTime2KTime(ts), TravelDir0);

		f32 te = f32(key)/f32(totalKeys);
		Vec3 TravelDir1;
		pRootCtl->GetP( rGAH.NTime2KTime(te), TravelDir1);

		f32 vertAngle = 0.0f;

		f32 dz = TravelDir1.z - TravelDir0.z;
		f32 dy = TravelDir1.y - TravelDir0.y;

		if(fabs(dy) < 0.01f)
		{
			if(fabs(dz) < 0.01f)
				vertAngle = .0f;
			else if(dz > 0.01f)
				vertAngle = gf_PI / 2.0f;
			else if(dz < -0.01f)
				vertAngle = -gf_PI / 2.0f;
		}
		else
			vertAngle = -atan(dz/dy);
		rGAH.m_arrLocoVertAnglePMG[k] = vertAngle; 
	}

	if (totalKeys>1)
		rGAH.m_arrLocoVertAnglePMG[0]=rGAH.m_arrLocoVertAnglePMG[1];
}

static f32 ComputeTravalDistPMG(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, const int GAID, IController* pRootCtl)
{
	assert(pRootCtl);

	const GlobalAnimationHeaderPMG::SParamPMG& paramPMG = pmgHeader.m_paramSpecPMG[paramInd];

	QuatT qt[NumKeys];
	f32 fTotalDist=0.0f;
	for(uint32 i=0; i<NumKeys; i++)
	{
		f32 t = paramPMG.startKey + (paramPMG.endKey - paramPMG.startKey) * f32(i)/(NumKeys-1) ;
		pRootCtl->GetOP( rGAH.NTime2KTime(f32(t)), qt[i].q,qt[i].t );
	}

	for(uint32 i=0; i<(NumKeys-1); i++)
		fTotalDist  += (qt[i+1].t-qt[i].t).GetLength();

	return fTotalDist;
}



static f32 ComputeSpeedPMG(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, const int GAID, IController* pRootCtl)
{
	assert(pRootCtl);

	const GlobalAnimationHeaderPMG::SParamPMG& paramPMG = pmgHeader.m_paramSpecPMG[paramInd];

	QuatT qt[NumKeys];
	f64 fTotalSpeed=0.0f;
	for(uint32 i=0; i<NumKeys; i++)
	{
		f32 t = paramPMG.startKey + (paramPMG.endKey - paramPMG.startKey) * f32(i)/(NumKeys-1) ;
		pRootCtl->GetOP( rGAH.NTime2KTime(f32(t)), qt[i].q,qt[i].t );
	}

	for(uint32 i=0; i<(NumKeys-1); i++)
		fTotalSpeed  += (qt[i+1].t-qt[i].t).GetLength();

	f32 fDuration = rGAH.m_fEndSec - rGAH.m_fStartSec;
	assert(fDuration > .0f);

	return f32(fTotalSpeed / fDuration);
}


static f32 ComputeTurnSpeedPMG(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl)
{
	assert(pRootCtl);

	const GlobalAnimationHeaderPMG::SParamPMG& paramPMG = pmgHeader.m_paramSpecPMG[paramInd];

	QuatT qt[NumKeys];
	for(uint32 i=0; i<NumKeys; i++)
	{
		f32 t = paramPMG.startKey + (paramPMG.endKey - paramPMG.startKey) * f32(i)/(NumKeys-1) ;
		pRootCtl->GetOP( rGAH.NTime2KTime(f32(t)), qt[i].q,qt[i].t );
	}

	f64 fTotalTurn=0.0f;
	for(uint32 i=0; i<(NumKeys-1); i++)
	{
		const Vec3r sDir = qt[i+0].q.GetColumn1();
		const Vec3r eDir = qt[i+1].q.GetColumn1();
		fTotalTurn += Ang3r::CreateRadZ(sDir, eDir);
	}

	f32 fDuration = rGAH.m_fEndSec-rGAH.m_fStartSec; 	//assert(fDuration>0.0f);
	return (f32(fTotalTurn)/fDuration);

}


static f32 ComputeTurnAnglePMG(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl)
{
	assert(pRootCtl);

	const GlobalAnimationHeaderPMG::SParamPMG& paramPMG = pmgHeader.m_paramSpecPMG[paramInd];

	QuatT qt[NumKeys];
	for(uint32 i=0; i<NumKeys; i++)
	{
		f32 t = paramPMG.startKey + (paramPMG.endKey - paramPMG.startKey) * f32(i)/(NumKeys-1) ;
		pRootCtl->GetOP( rGAH.NTime2KTime(f32(t)), qt[i].q,qt[i].t );
	}

	f64 fTotalTurn=0.0f;
	for(uint32 i=0; i<(NumKeys-1); i++)
	{
		const Vec3r sDir = qt[i+0].q.GetColumn1();
		const Vec3r eDir = qt[i+1].q.GetColumn1();
		fTotalTurn += Ang3r::CreateRadZ(sDir, eDir);
	}

	f32 fDuration = rGAH.m_fEndSec-rGAH.m_fStartSec; 	//assert(fDuration>0.0f);
	return f32(fTotalTurn);
}


static f32 ComputeStrifeAnglePMG(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl)
{
	assert(pRootCtl);

	const GlobalAnimationHeaderPMG::SParamPMG& paramPMG = pmgHeader.m_paramSpecPMG[paramInd];

	QuatT qt[NumKeys];
	Vec3r movedir[NumKeys-1];
	f64 turn[NumKeys-1];
	for(uint32 i=0; i<NumKeys; i++)
	{
		f32 t = paramPMG.startKey + (paramPMG.endKey - paramPMG.startKey) * f32(i)/(NumKeys-1) ;
		pRootCtl->GetOP( rGAH.NTime2KTime(f32(t)), qt[i].q,qt[i].t );
	}
	for(uint32 i=0; i<(NumKeys-1); i++)
	{
		movedir[i] = (qt[i+1].t-qt[i].t).GetNormalizedSafe( Vec3r(0,1,0) );
	}
	for(uint32 i=0; i<(NumKeys-1); i++)
	{
		const Vec3r sDir = qt[i+0].q.GetColumn1();
		const Vec3r eDir = qt[i+1].q.GetColumn1();
		turn[i]	= Ang3r::CreateRadZ(sDir, eDir);
	}

	f64 fSumTurn=0.0f;
	Vec3r vTotalMDir=Vec3r(ZERO);
	for(uint32 i=0; i<(NumKeys-1); i++)
	{
		fSumTurn    += turn[i];
		Vec3r mdir   = movedir[i]*Matrix33r::CreateRotationZ( f32(fSumTurn) );
		vTotalMDir  += mdir;
	}

	vTotalMDir.NormalizeSafe( Vec3r(0,1,0) );
	f32 fDuration = rGAH.m_fEndSec - rGAH.m_fStartSec;
	assert(fDuration > .0f);

	return f32(Ang3r::CreateRadZ(Vec3r(0,1,0),vTotalMDir));
}

static f32 ComputeSlopePMG(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, IController* pRootCtl)
{
	assert(pRootCtl);
	Vec3 TravelDir0;
	pRootCtl->GetP(rGAH.NTime2KTime(0),TravelDir0);

	Vec3 TravelDir1;
	pRootCtl->GetP( rGAH.NTime2KTime(1),TravelDir1);

	assert( (TravelDir1-TravelDir0).GetLength()>0.01f );

	f32 dz = TravelDir1.z - TravelDir0.z;
	f32 dy = TravelDir1.y - TravelDir0.y;

	f32 fParam_VertAngle=0;
	if(fabs(dy) < 0.01f)
	{
		if(fabs(dz) < 0.01f)
			fParam_VertAngle = .0f;
		else if(dz > 0.01f)
			fParam_VertAngle = gf_PI / 2.0f;
		else if(dz < -0.01f)
			fParam_VertAngle = -gf_PI / 2.0f;
	}
	else
		fParam_VertAngle = -atan(dz/dy);

	return fParam_VertAngle;
}

// polarCoord:
//    x: x-y plane angle
//    y: y-z plane angle
//    z: r, the length
void CAnimationSet::ComputerAimPoseCoord(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, Vec3& polarCoord)
{
	uint32 numJoints = m_pModel->m_ModelSkeleton.GetJointCount();
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;
	CModelJoint* pModelJoint = &pModelSkeleton->m_arrModelJoints[0];

	QuatT relQuatT[256];
	QuatT absQuatT[256];
	for (uint32 j=0; j<numJoints; j++)
	{
		relQuatT[j].q = pModelSkeleton->m_poseData.m_jointsRelative[j].q;
		relQuatT[j].t = pModelSkeleton->m_poseData.m_jointsRelative[j].t;
		IController* pController = rGAH.GetControllerByJointCRC32(pModelJoint[j].m_nJointCRC32);
		IF (pController, true)
			Status4 rStatus = pController->GetOP( rGAH.NTime2KTime(0.0f), relQuatT[j].q, relQuatT[j].t );
	} 

	//we assume that all CHRs are single root objects
	absQuatT[0] = relQuatT[0];
	for (uint32 i=1; i<numJoints; i++)
		absQuatT[i]	= absQuatT[pModelJoint[i].m_idxParent] * relQuatT[i];

	int32 jointID = pModelSkeleton->GetJointIDByName("weapon_bone");
	assert(jointID != -1);

	Quat qAbs = absQuatT[jointID].q;
	Vec3 aimPosVec = qAbs.GetColumn1(); // Y axis

	polarCoord.x = atan2f(-aimPosVec.x,aimPosVec.y);
	f32 s,c; sincos_tpl(polarCoord.x,&s,&c);
	polarCoord.y = atan2f(aimPosVec.z,-aimPosVec.x*s+aimPosVec.y*c);
	polarCoord.z = aimPosVec.GetLength();
}

void CAnimationSet::ComputerWeaponBonePosition(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, Vec3& pos)
{
	uint32 numJoints = m_pModel->m_ModelSkeleton.GetJointCount();
	CModelSkeleton* pModelSkeleton = &m_pModel->m_ModelSkeleton;
	CModelJoint* pModelJoint = &pModelSkeleton->m_arrModelJoints[0];

	QuatT relQuatT[256];
	QuatT absQuatT[256];
	for (uint32 j=0; j<numJoints; j++)
	{
		relQuatT[j].q = pModelSkeleton->m_poseData.m_jointsRelative[j].q;
		relQuatT[j].t = pModelSkeleton->m_poseData.m_jointsRelative[j].t;
		IController* pController = rGAH.GetControllerByJointCRC32(pModelJoint[j].m_nJointCRC32);
		IF (pController, true)
			Status4 rStatus = pController->GetOP( rGAH.NTime2KTime(0.0f), relQuatT[j].q, relQuatT[j].t );
	} 

	//we assume that all CHRs are single root objects
	absQuatT[0] = relQuatT[0];
	for (uint32 i=1; i<numJoints; i++)
		absQuatT[i]	= absQuatT[pModelJoint[i].m_idxParent] * relQuatT[i];

	int32 jointID = pModelSkeleton->GetJointIDByName("weapon_bone");
	assert(jointID != -1);

	pos = absQuatT[jointID].t;
}

bool CAnimationSet::IsInPMGParamNameMap(const uint32 globalAnimID, const char* name) const
{
	GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[globalAnimID];
	uint32 res = stl::find_in_map(rGlobalAnimHeader.m_paramNamePMGMap, name, -1);

	return res != -1? true:false;
}

uint32 CAnimationSet::GetPMGParamDim(const uint32 globalAnimID) const
{
	GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[globalAnimID];
	return (uint32)rGlobalAnimHeader.m_paramSpecPMG.size();
}

void CAnimationSet::CalculateLocatorParameters( const GlobalAnimationHeaderPMG& pmgHeader)
{
	uint32 pmg= pmgHeader.IsAssetPMG();
	if (pmg)
	{
		if (pmgHeader.IsAssetCreated())
		{
			//its a locomotion group
			uint32 numExamples = pmgHeader.m_arrBSAnimationsPMG.size();
			uint32 PMG_OK = numExamples;
			for (uint32 g=0; g<numExamples; g++)
			{
				const char* aname = pmgHeader.m_arrBSAnimationsPMG[g].m_strAnimName;

				int32 id=GetAnimIDByName(aname);
				if (id<0)
				{
					assert(0);
					if (Console::GetInst().ca_AnimWarningLevel>0)
						AnimFileWarning(m_pModel->GetModelFilePath(),"locomotion group '%s' is invalid! The animation '%s' is not in the CAL-file.", pmgHeader.GetFilePath(), aname);
					PMG_OK=0;
				}
				else
				{
					int32 gaid=m_arrAnimations[id].m_nGlobalAnimId;
					GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[gaid];

					if (rGAH.IsAssetNotFound() || rGAH.m_fStartSec<0 || rGAH.m_fEndSec<0)
					{
						if (Console::GetInst().ca_AnimWarningLevel>0)
							AnimFileWarning(m_pModel->GetModelFilePath(),"locomotion group '%s' is invalid! The the asset for animation '%s' does not exist.", pmgHeader.GetFilePath(), aname);
						PMG_OK=0;
					} 
				}
			}

			if (PMG_OK)
			{
				const DynArray<BSAnimationPMG> & bsAnimations = pmgHeader.m_arrBSAnimationsPMG;

				uint32 numAnims = (uint32)bsAnimations.size();
				for (uint32 i=0; i<numAnims; i++)
				{
					const char* aname = bsAnimations[i].m_strAnimName;
					int32 globalID = GetGlobalIDByName(aname);
					assert(globalID >= 0);

					GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[globalID];
					IController* pRootCtl = GAH_GetRootController(rGAH,globalID,m_pModel,bsAnimations[i].m_strAnimName);
					assert(pRootCtl);

					ComputeLocatorSpeedPMG(pmgHeader, rGAH, globalID, pRootCtl, i);
					ComputeLocatorStrifeAnglePMG(pmgHeader, rGAH, globalID, pRootCtl, i);
					ComputeLocatorTurnSpeedPMG(pmgHeader, rGAH, globalID, pRootCtl, i);
					ComputeLocatorVerticalMovPMG(pmgHeader, rGAH, globalID, pRootCtl, i);
				}
			}
		}
	}
}


// Dummy PMG for linear blending test in Character editor
void CAnimationSet::SetDummyPMGAnimName(const uint32 globalAnimID, const char* animNameA, const char* animNameB)
{
	GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[globalAnimID];
	assert( stricmp(rGlobalAnimHeader.GetFilePath(), "_PMG_Dummy_PATH") == 0);

	BSAnimationPMG animName;
	animName.m_strAnimName = animNameA;
	rGlobalAnimHeader.m_arrBSAnimationsPMG[0] = animName;

	animName.m_strAnimName = animNameB;
	rGlobalAnimHeader.m_arrBSAnimationsPMG[1] = animName;

	CalculateLocatorParameters(rGlobalAnimHeader);

}

string CAnimationSet::GetPMGParamName(const uint32 globalAnimID, const uint32 paramInd) const
{
	GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[globalAnimID];
	return rGlobalAnimHeader.m_paramSpecPMG[paramInd].name;

}
void CAnimationSet::GetPMGParamLimits(const uint32 globalAnimID, const uint32 paramInd, f32& minV, f32& maxV) const
{
	minV = .0f;
	maxV = .0f;
	GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[globalAnimID];
	if(paramInd != -1)
	{
		minV = rGlobalAnimHeader.m_paramSpecPMG[paramInd].minValue;
		maxV = rGlobalAnimHeader.m_paramSpecPMG[paramInd].maxValue;
	}

}

f32 CAnimationSet::CalculatePMGParameters(const uint32 animInd, const uint32 paramInd, const string paramName, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, const int GAID, IController* pRootCtl)
{	
	uint32 iSize = (uint32)pmgHeader.m_userParam[animInd].size();

	if(paramName == "MoveSpeed")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "MoveSpeed")
				return pmgHeader.m_userParam[animInd][i].val;
		}
		return ComputeSpeedPMG(paramInd, pmgHeader, rGAH, GAID, pRootCtl);
	}
	else if(paramName == "TurnSpeed")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "TurnSpeed")
				return pmgHeader.m_userParam[animInd][i].val;
		}
		return ComputeTurnSpeedPMG(paramInd, pmgHeader, rGAH, GAID, pRootCtl);
	}
	else if(paramName == "Slope")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "Slope")
				return pmgHeader.m_userParam[animInd][i].val;
		}	
		return ComputeSlopePMG(paramInd, pmgHeader,rGAH, GAID, pRootCtl);

	}
	else if(paramName == "TravelDir")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "TravelDir")
				return pmgHeader.m_userParam[animInd][i].val;
		}	
		return ComputeStrifeAnglePMG(paramInd, pmgHeader, rGAH, GAID, pRootCtl);
	}
	else if(paramName == "TravelDist")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "TravelDist")
				return pmgHeader.m_userParam[animInd][i].val;
		}	
		return ComputeTravalDistPMG(paramInd, pmgHeader, rGAH, GAID, pRootCtl);
	}
	else if(paramName == "TurnAngle")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "TurnAngle")
				return pmgHeader.m_userParam[animInd][i].val;
		}	
		return ComputeTurnAnglePMG(paramInd, pmgHeader, rGAH, GAID, pRootCtl);
	}
	else if(paramName == "IK_POSITION_X")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "IK_POSITION_X")
				return pmgHeader.m_userParam[animInd][i].val;
		}	
		Vec3 pos;
		ComputerWeaponBonePosition(paramInd, pmgHeader, rGAH, GAID, pos);
		return pos.x;
	}
	else if(paramName == "IK_POSITION_Y")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "IK_POSITION_Y")
				return pmgHeader.m_userParam[animInd][i].val;
		}	
		Vec3 pos;
		ComputerWeaponBonePosition(paramInd, pmgHeader, rGAH, GAID, pos);
		return pos.y;		
	}
	else if(paramName == "IK_POSITION_Z")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "IK_POSITION_Z")
				return pmgHeader.m_userParam[animInd][i].val;
		}	
		Vec3 pos;
		ComputerWeaponBonePosition(paramInd, pmgHeader, rGAH, GAID, pos);
		return pos.z;
	}
	else if(paramName == "AIMPOSE_XY_ANGLE")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "AIMPOSE_XY_ANGLE")
				return pmgHeader.m_userParam[animInd][i].val;
		}	
		Vec3 polarCoord;
		ComputerAimPoseCoord(paramInd, pmgHeader, rGAH, GAID, polarCoord);
		return polarCoord.x;
	}
	else if(paramName == "AIMPOSE_YZ_ANGLE")
	{
		for(uint32 i=0; i< iSize; ++i)
		{
			if(pmgHeader.m_userParam[animInd][i].name == "AIMPOSE_YZ_ANGLE")
				return pmgHeader.m_userParam[animInd][i].val;
		}	
		Vec3 polarCoord;
		ComputerAimPoseCoord(paramInd, pmgHeader, rGAH, GAID, polarCoord);
		return polarCoord.y;
	}
	else
		return 0.0f;
}

//----------------------------------------------------------------------------------
//----      check if all animation-assets in a locomotion group are valid       ----
//----------------------------------------------------------------------------------
void CAnimationSet::VerifyPMGs()
{
	uint32 numPMG = m_arrAnimations.size();

	f32* pA = (f32*)alloca( MAX_PMG_PARAM_DIM*MAX_PMG_ANIMS*sizeof(f32) );
	f32* pR = (f32*)alloca( MAX_PMG_ANIMS*MAX_PMG_ANIMS*sizeof(f32) );
	f32* pScale = (f32*)alloca( MAX_PMG_ANIMS*sizeof(f32) );

	f32* pAllParam = (f32*)alloca(MAX_PMG_ANIMS*MAX_PMG_PARAM_DIM*sizeof(f32));
	f32* pParamTmp = (f32*)alloca(MAX_PMG_PARAM_DIM*sizeof(f32));
	f32* pVirtual = (f32*)alloca( MAX_PMG_PARAM_DIM*sizeof(f32) );
	f32* pT = (f32*)alloca( MAX_PMG_PARAM_DIM*sizeof(f32) );

	for (uint32 i=0; i<numPMG; i++)
	{
		if (m_arrAnimations[i].m_nAssetType!=PMG_File)
			continue;

		uint32 GlobalAnimationID  = m_arrAnimations[i].m_nGlobalAnimId;
		GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[GlobalAnimationID];

		if ( stricmp(rGlobalAnimHeader.GetFilePath(), "_PMG_Dummy_PATH") == 0)
		{
			rGlobalAnimHeader.OnAssetPMGValid();
			continue;
		}

		uint32 pmg= rGlobalAnimHeader.IsAssetPMG();
		if (pmg)
		{
			if (rGlobalAnimHeader.IsAssetCreated())
			{
				//its a locomotion group
				uint32 numExamples = rGlobalAnimHeader.m_arrBSAnimationsPMG.size();
				uint32 PMG_OK = numExamples;
				for (uint32 g=0; g<numExamples; g++)
				{
					const char* aname = rGlobalAnimHeader.m_arrBSAnimationsPMG[g].m_strAnimName;

					int32 id=GetAnimIDByName(aname);
					if (id<0)
					{
						assert(0);
						if (Console::GetInst().ca_AnimWarningLevel>0)
							AnimFileWarning(m_pModel->GetModelFilePath(),"locomotion group '%s' is invalid! The animation '%s' is not in the CAL-file.", m_arrAnimations[i].GetAnimName(),aname);
						PMG_OK=0;
					}
					else
					{
						int32 gaid=m_arrAnimations[id].m_nGlobalAnimId;
						GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[gaid];

						if (rGAH.IsAssetNotFound() || rGAH.m_fStartSec<0 || rGAH.m_fEndSec<0)
						{
							if (Console::GetInst().ca_AnimWarningLevel>0)
								AnimFileWarning(m_pModel->GetModelFilePath(),"locomotion group '%s' is invalid! The the asset for animation '%s' does not exist.", m_arrAnimations[i].GetAnimName(),aname);
							PMG_OK=0;
						} 
					}
				}

				if (PMG_OK)
				{
					rGlobalAnimHeader.OnAssetPMGValid();
					rGlobalAnimHeader.OnAssetLMGValid();
					DynArray<BSAnimationPMG> & bsAnimations = rGlobalAnimHeader.m_arrBSAnimationsPMG;

					uint32 numAnims = (uint32)bsAnimations.size();
					uint32 numParamDim = (uint32) rGlobalAnimHeader.m_paramNamePMGMap.size();
					uint32 numVirtualExamp = (uint32) rGlobalAnimHeader.m_virtualExmps.size();
					uint32 N = numAnims + numVirtualExamp;
					uint32 D = numParamDim + 1;

					 
				
					fMatrix paramTmp( 1, D, pParamTmp);

					const DynArray<GlobalAnimationHeaderPMG::SParamPMG>& paramSpecPMG = rGlobalAnimHeader.m_paramSpecPMG;

					// Always calculate Locator pamrameters
					CalculateLocatorParameters(rGlobalAnimHeader);

					//------------------------------------------------------------------------------
					// Calculate PMG parameters
					for (uint32 k=0; k<numAnims; ++k)
					{
						const char* aname = bsAnimations[k].m_strAnimName;
						int32 globalID = GetGlobalIDByName(aname);
						assert(globalID >= 0);

						GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[globalID];
						IController* pRootCtl = GAH_GetRootController(rGAH,globalID,m_pModel,bsAnimations[k].m_strAnimName);
						assert(pRootCtl);

						for(uint32 j=0; j<numParamDim; ++j)
						{
							paramTmp(0, j) = CalculatePMGParameters(i, j, paramSpecPMG[j].name, rGlobalAnimHeader, rGAH, globalID, pRootCtl);
							paramTmp(0, j) *= paramSpecPMG[j].scale;

						}

						paramTmp(0, numParamDim) = 1.0f; // homogeneous coordinates

						memcpy(pAllParam + k*D, paramTmp.GetData(), D*sizeof(f32));
					}

					//------------------------------------------------------------------------------
					// Automatically generate virtual-examples
					// Please keep this code

					//------------------------------------------------------------------------------
					// Add in-between examples
					/*const std::vector<fMatrix>& params = rGlobalAnimHeader.m_ami.GetParameters();
					uint32 numParams = (uint32) params.size();
					uint32 numDim = (uint32) rGlobalAnimHeader.m_ami.GetParamDimension();

					std::vector<f32> avgDist(rGlobalAnimHeader.m_ami.GetParamDimension());
					for(uint32 i=0; i<numDim; ++i)
					{
					f32 avgD = .0f;
					for(uint32 j=0; j<numParams; ++j)
					{
					avgD += params[j](0, i);
					}
					avgD /= numParams;

					}

					for(uint32 i=0; i<numParams; ++i)
					{
					f32 dMin = 1e+20f;
					uint32 minInd = -1;

					for(uint32 j=0; j<numParams; ++j)
					{
					if(j != i)
					{
					fMatrix diff = params[i] - params[j];
					f32 d = diff.SqrtDist();
					if(d < dMin)
					{
					dMin = d;
					minInd = j;
					}
					}
					}
					}*/


					//------------------------------------------------------------------------------
					// Deal with VirtualExamples


					DynArray<GlobalAnimationHeaderPMG::SVirtualExamp>& virtualExamp = rGlobalAnimHeader.m_virtualExmps;

					uint32 numPExamp = (uint32)virtualExamp.size(); // predefined virtual examples
				
					for(uint32 k=0; k<numPExamp; ++k)
					{
						f32 a = virtualExamp[k].alpha;
						uint32 ind1 = virtualExamp[k].ind1;
						uint32 ind2 = virtualExamp[k].ind2;
						fMatrix p(1, D, pVirtual);

						fMatrix exmap1(1, D, pAllParam + ind1*D);
						p = exmap1;
						p *= a;

						fMatrix exmap2(1, D, pAllParam + ind2*D);
						fMatrix t(1, D, pT);
						t = exmap2;
						t *= (1-a);

						p += t;

						memcpy( pAllParam + (numAnims + k)*D, p.GetData(), D*sizeof(f32) );
					}

					//------------------------------------------------------------------------------
					// Please reserve the following code for possible further virtual-example test

					//const std::vector<GlobalAnimationHeaderCAF::SVirtualEdge>& virtualEdgeList = rGlobalAnimHeader.m_edgeList;
					//uint32 numEdges = (uint32)virtualEdgeList.size();
					//if(numEdges > 0)
					//{
					//	const uint32 divLevelH = 1;
					//	const uint32 divLevelV = 0;
					//	f32 stepH =1.0f / (divLevelH+1); 
					//	f32 stepV =1.0f / (divLevelV+1); 
					//	for(uint32 i=0; i<numEdges; ++i)
					//	{
					//		for(uint32 j=0; j<divLevelH; ++j)
					//		{
					//			f32 alpha = stepH * (j+1);
					//			fMatrix p = params[ virtualEdgeList[i].ind1 ] * alpha + params[ virtualEdgeList[i].ind2 ] * (1-alpha);
					//			rGlobalAnimHeader.m_ami.AddParams(p);

					//			GlobalAnimationHeaderCAF::SVirtualExamp examp;
					//			examp.ind1 = virtualEdgeList[i].ind1;
					//			examp.ind2 = virtualEdgeList[i].ind2;
					//			examp.alpha = alpha;
					//			virtualExamp.push_back(examp);
					//		}						
					//	}

					//	const std::vector<GlobalAnimationHeaderCAF::SVirtualEdge>& subdivideEdgeList = rGlobalAnimHeader.m_subdivideEdgeList;
					//	uint32 numSubdivideEdges = (uint32)subdivideEdgeList.size();
					//	std::vector< std::vector<uint32> > virtualSubEdgeList(numSubdivideEdges); // keep the ids of virtual-examples

					//	// normal edge subdivision first, remember the ids of virtual-examples on each edge
					//	for(uint32 i=0; i<numSubdivideEdges; ++i)
					//	{
					//		for(uint32 j=0; j<divLevelV; ++j)
					//		{
					//			f32 alpha = stepV * (j+1);
					//			fMatrix p = params[ subdivideEdgeList[i].ind1 ] * alpha + params[ subdivideEdgeList[i].ind2 ] * (1-alpha);
					//			rGlobalAnimHeader.m_ami.AddParams(p);

					//			GlobalAnimationHeaderCAF::SVirtualExamp examp;
					//			examp.ind1 = subdivideEdgeList[i].ind1;
					//			examp.ind2 = subdivideEdgeList[i].ind2;
					//			examp.alpha = alpha;
					//			virtualExamp.push_back(examp);

					//			uint32 id = virtualExamp.size() - 1;
					//			virtualSubEdgeList[i].push_back(id);
					//		}						
					//	}

					//	// Now generate the in-between virtual-examples upon virtual-examples
					//	// Please keep this code

					//	//for(uint32 i=0; i<numSubdivideEdges-1; ++i)
					//	//{
					//	//	f32 stepH2 = stepH, divLevelH2 =divLevelH;
					//	//	if( i==2 || i==5 )
					//	//	{
					//	//		divLevelH2 = 2;
					//	//		stepH2 = 1.0f / (divLevelH2 + 1);
					//	//	}
					//	//
					//	//	uint32 vsize = virtualSubEdgeList[i].size();
					//	//	for(uint32 j=0; j<vsize; ++j)
					//	//	{
					//	//		uint32 ind1 = virtualSubEdgeList[i][j];
					//	//		uint32 ind2 = virtualSubEdgeList[i+1][j];

					//	//		for(uint32 k=0; k<divLevelH2; ++k)
					//	//		{
					//	//			f32 alpha = stepH2 * (k+1);
					//	//			fMatrix p = params[ ind1 + numAnims ] * alpha + params[ ind2 + numAnims  ] * (1-alpha);
					//	//			rGlobalAnimHeader.m_ami.AddParams(p);

					//	//			GlobalAnimationHeaderCAF::SVirtualExamp examp;
					//	//			examp.ind1 = ind1; 
					//	//			examp.ind2 = ind2;
					//	//			examp.alpha = alpha;
					//	//			virtualExamp.push_back(examp);
					//	//		}
					//	//	}
					//	//}
					//}


					//------------------------------------------------------------------------------
					// Construct SDI
					f32* pParams = pAllParam;
					

					CSDI ami(N, D, pParams, pA, pR, pScale);
					ami.ConstructSDI();

					rGlobalAnimHeader.m_paramSDI.WriteData(N, D, pParams, pA, pR, pScale);
					rGlobalAnimHeader.m_paramSDI.ReadData(N, D, pParams, pA, pR, pScale);
				}

				uint32 numAssets = rGlobalAnimHeader.m_arrBSAnimationsPMG.size();
				for (uint32 a=0; a<numAssets; a++)
				{
					const char* name = rGlobalAnimHeader.m_arrBSAnimationsPMG[a].m_strAnimName; //0
					int32 globalID = GetGlobalIDByName(name);
					assert(globalID>=0);
					if (globalID < 0)
						continue;
					uint32 OnDemand=g_AnimationManager.m_arrGlobalCAF[globalID].IsAssetOnDemand();
					if (OnDemand)
					{
						if ( g_AnimationManager.m_arrGlobalCAF[globalID].IsAssetLoaded() )
							g_AnimationManager.UnloadAnimationCAF(globalID);
					}
				}
			}
		}
	}
}

uint32 CAnimationSet::numMorphTargets() const  
{
	CModelMesh* pModelMesh = m_pModel->GetModelMesh(0);
	if (pModelMesh==0)
		return 0;

	return pModelMesh->m_morphTargets.size();
};

const char* CAnimationSet::GetNameMorphTarget (int nMorphTargetId) 
{
	if (nMorphTargetId< 0)
		return "!NEGATIVE MORPH TARGET ID!";

	if ((uint32)nMorphTargetId >= m_pModel->GetModelMesh(0)->m_morphTargets.size())
		return "!MORPH TARGET ID OUT OF RANGE!";

	return m_pModel->GetModelMesh(0)->m_morphTargets[nMorphTargetId]->m_name.c_str();
};



// prepares to load the specified number of CAFs by reserving the space for the controller pointers
void CAnimationSet::prepareLoadCAFs (uint32 nReserveAnimations)
{
	nReserveAnimations += m_arrAnimations.size();

	//uint32 numJoints = m_pModel->m_ModelSkeleton.m_arrModelJoints.size();
	//for (uint32 i=0; i<numJoints; i++)
	//	m_pModel->m_ModelSkeleton.m_arrModelJoints[i].m_arrControllersMJoint.reserve (nReserveAnimations);

	m_arrAnimations.reserve (nReserveAnimations);
	//	m_arrAnimByGlobalId.reserve (nReserveAnimations);
	//	m_arrAnimByLocalName.reserve (nReserveAnimations);
}



const ModelAnimationHeader* CAnimationSet::GetModelAnimationHeader(int32 i)
{
	DEFINE_PROFILER_FUNCTION();

	int32 numAnimation = (int32)m_arrAnimations.size();
	if (i<0 || i>=numAnimation)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"Invalid Animation ID: %d",i);
		return 0;
	}
	return &m_arrAnimations[i];  //the animation-asset exists
}




//----------------------------------------------------------------------------------
// Returns the index of the animation in the set, -1 if there's no such animation
//----------------------------------------------------------------------------------
int CAnimationSet::GetAnimIDByName( const char* szAnimationName)
{ 
	if (szAnimationName==0)
		return -1;

	//this is probably the slowest function in the system.
	//TODO: needs some heavy optimisation in the future 
	if (szAnimationName[0] == '#')
	{	
		//search for morph-names
		return int(m_arrAnimations.size() + m_pModel->m_arrModelMeshes[0].FindMorphTarget(szAnimationName));
	}
	else
	{
		//search for animation-names 
		/*
		uint32 numAnims=m_arrAnimations.size();
		for (uint32 i=0; i<numAnims; i++)
		{
		if (stricmp(m_arrAnimations[i].m_strAnimName.c_str(),szAnimationName)==0)
		return i;
		}
		*/

		return m_AnimationHashMap.GetValue(szAnimationName);
	}
}


// Returns the given animation name
const char* CAnimationSet::GetNameByAnimID ( int nAnimationId)
{
	if (m_pModel->m_ObjectType==CGA)
	{
		if (nAnimationId < 0)
			return "";
		return m_arrAnimations[nAnimationId].GetAnimName();
	} 
	else
	{
		if (nAnimationId >=0)
		{
			if (nAnimationId < (int)m_arrAnimations.size())
				return m_arrAnimations[nAnimationId].GetAnimName();
			nAnimationId -= (int)m_arrAnimations.size();
			if (nAnimationId < (int)m_pModel->m_arrModelMeshes[0].m_morphTargets.size())
				return m_pModel->m_arrModelMeshes[0].m_morphTargets[nAnimationId]->m_name.c_str();
			return "!ANIMATION ID OUT OF RANGE!";
		}
		else
			return "!NEGATIVE ANIMATION ID!";
	}

}

//------------------------------------------------------------------------------

f32 CAnimationSet::GetSpeed(int nAnimationId)
{
	int32 numAnimation = (int32)m_arrAnimations.size();
	if (nAnimationId<0 || nAnimationId>=numAnimation)
	{
		AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index used in function GetSpeed '%d'", nAnimationId);
		return -1;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim==0)
		return -1;

	uint32 GlobalAnimationID			= anim->m_nGlobalAnimId;
	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];
	f32	fDistance = rGlobalAnimHeader.m_fDistance;
	f32	fDuration = rGlobalAnimHeader.m_fEndSec - rGlobalAnimHeader.m_fStartSec;
	f32	msec      = fDistance/fDuration;
	return msec;
}

f32 CAnimationSet::GetSlope(int nAnimationId)
{
	int32 numAnimation = (int32)m_arrAnimations.size();
	if (nAnimationId<0 || nAnimationId>=numAnimation)
	{
		AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index used in function GetSpeed '%d'", nAnimationId);
		return 0;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim==0)
		return 0;

	uint32 GlobalAnimationID			= anim->m_nGlobalAnimId;
	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];
	return rGlobalAnimHeader.m_fSlope;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

GlobalAnimationHeaderCAF* CAnimationSet::GetGAH_CAF(const char* AnimationName)
{ 
	int32 subAnimID = GetAnimIDByName(AnimationName);
	if (subAnimID<0)
		return 0; //error -> name not found
	return GetGAH_CAF(subAnimID);
}
GlobalAnimationHeaderAIM* CAnimationSet::GetGAH_AIM(const char* AnimationName)
{ 
	int32 subAnimID = GetAnimIDByName(AnimationName);
	if (subAnimID<0)
		return 0; //error -> name not found
	return GetGAH_AIM(subAnimID);
}

GlobalAnimationHeaderLMG* CAnimationSet::GetGAH_LMG(const char* AnimationName)
{ 
	int32 subAnimID = GetAnimIDByName(AnimationName);
	if (subAnimID<0)
		return 0; //error -> name not found
	return GetGAH_LMG(subAnimID);
}

GlobalAnimationHeaderPMG* CAnimationSet::GetGAH_PMG(const char* AnimationName)
{ 
	int32 subAnimID = GetAnimIDByName(AnimationName);
	if (subAnimID<0)
		return 0; //error -> name not found
	return GetGAH_PMG(subAnimID);
}





GlobalAnimationHeaderCAF* CAnimationSet::GetGAH_CAF(int nAnimationId)
{ 
	int32 numAnimation = (int32)m_arrAnimations.size();
	if ((nAnimationId < 0) || (nAnimationId >= numAnimation))
	{
#if !defined(__SPU__)
		AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index '%d'", nAnimationId);
#endif
		return NULL;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim == NULL)
		return NULL;
	if (anim->m_nAssetType==CAF_File)
		return &g_AnimationManager.m_arrGlobalCAF[anim->m_nGlobalAnimId];
	return 0;
}

GlobalAnimationHeaderAIM* CAnimationSet::GetGAH_AIM(int nAnimationId)
{ 
	int32 numAnimation = (int32)m_arrAnimations.size();
	if ((nAnimationId < 0) || (nAnimationId >= numAnimation))
	{
#if !defined(__SPU__)
		AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index '%d'", nAnimationId);
#endif
		return NULL;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim == NULL)
		return NULL;
	if (anim->m_nAssetType==AIM_File)
		return &g_AnimationManager.m_arrGlobalAIM[anim->m_nGlobalAnimId];
	return 0;
}


GlobalAnimationHeaderLMG* CAnimationSet::GetGAH_LMG(int nAnimationId)
{ 
	int32 numAnimation = (int32)m_arrAnimations.size();
	if ((nAnimationId < 0) || (nAnimationId >= numAnimation))
	{
#if !defined(__SPU__)
		AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index '%d'", nAnimationId);
#endif
		return NULL;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim == NULL)
		return NULL;
	if (anim->m_nAssetType==LMG_File)
		return &g_AnimationManager.m_arrGlobalLMG[anim->m_nGlobalAnimId];
	return 0;
}

GlobalAnimationHeaderPMG* CAnimationSet::GetGAH_PMG(int nAnimationId)
{ 
	int32 numAnimation = (int32)m_arrAnimations.size();
	if ((nAnimationId < 0) || (nAnimationId >= numAnimation))
	{
#if !defined(__SPU__)
		AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index '%d'", nAnimationId);
#endif
		return NULL;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim == NULL)
		return NULL;
	if (anim->m_nAssetType==PMG_File)
		return &g_AnimationManager.m_arrGlobalPMG[anim->m_nGlobalAnimId];
	return 0;
}


//------------------------------------------------------------------------------

Vec2 CAnimationSet::GetMinMaxSpeedAsset_msec(int32 animID )
{ 
	Vec2 MinMax(0,0);
	if (animID<0)
		return MinMax;

	GlobalAnimationHeaderCAF* pGlobalAnimHeaderCAF = GetGAH_CAF(animID);
	if (pGlobalAnimHeaderCAF)
	{
		if (pGlobalAnimHeaderCAF->IsAssetCreated()==0)
		{
			AnimFileWarning(m_pModel->GetModelFilePath(),"CryAnimation\\CAnimationSet::GetMinMaxSpeed_msec(): Animation %d is not loaded", animID);
			return MinMax;
		}
		return Vec2(0,0); 
	}

	GlobalAnimationHeaderLMG* pGlobalAnimHeaderLMG = GetGAH_LMG(animID);
	if (pGlobalAnimHeaderLMG)
	{
		if (pGlobalAnimHeaderLMG->IsAssetCreated()==0)
		{
			AnimFileWarning(m_pModel->GetModelFilePath(),"CryAnimation\\CAnimationSet::GetMinMaxSpeed_msec(): Animation %d is not loaded", animID);
			return MinMax;
		}
		f32 minspeed=+9999.0f;
		f32 maxspeed=-9999.0f;
		uint32 subCount = pGlobalAnimHeaderLMG->m_arrBSAnimations.size();
		assert(subCount < 40);
		for (uint32 i = 0; i < subCount; i++)
		{
			const char* aname = pGlobalAnimHeaderLMG->m_arrBSAnimations[i].m_strAnimName;
			if (aname==0)
				return Vec2(0,0); 

			int32 subAnimID = GetAnimIDByName(aname);
			if (subAnimID<0)
				return Vec2(0,0); 

			GlobalAnimationHeaderCAF* rsubGlobalAnimHeader = GetGAH_CAF(subAnimID);
			if (rsubGlobalAnimHeader == 0)
				return Vec2(0,0); 

			if (minspeed>rsubGlobalAnimHeader->m_fSpeed) 
				minspeed=rsubGlobalAnimHeader->m_fSpeed;
			if (maxspeed<rsubGlobalAnimHeader->m_fSpeed) 
				maxspeed=rsubGlobalAnimHeader->m_fSpeed;
		}
		return Vec2(minspeed,maxspeed); 
	}


	GlobalAnimationHeaderPMG* pGlobalAnimHeaderPMG = GetGAH_PMG(animID);
	if (pGlobalAnimHeaderPMG)
	{
		if (pGlobalAnimHeaderPMG->IsAssetCreated()==0)
		{
			AnimFileWarning(m_pModel->GetModelFilePath(),"CryAnimation\\CAnimationSet::GetMinMaxSpeed_msec(): Animation %d is not loaded", animID);
			return MinMax;
		}

		f32 minspeed=+9999.0f;
		f32 maxspeed=-9999.0f;
		uint32 subCount = pGlobalAnimHeaderPMG->m_arrBSAnimationsPMG.size();
		assert(subCount < 40);
		for (uint32 i = 0; i < subCount; i++)
		{
			const char* aname = pGlobalAnimHeaderPMG->m_arrBSAnimationsPMG[i].m_strAnimName;
			if (aname==0)
				return Vec2(0,0); 

			int32 subAnimID = GetAnimIDByName(aname);
			if (subAnimID<0)
				return Vec2(0,0); 

			GlobalAnimationHeaderCAF* rsubGlobalAnimHeader = GetGAH_CAF(subAnimID);
			if ((rsubGlobalAnimHeader == 0) || rsubGlobalAnimHeader->IsAssetLMGValid()==0)
				return Vec2(0,0); 

			if (minspeed>rsubGlobalAnimHeader->m_fSpeed) 
				minspeed=rsubGlobalAnimHeader->m_fSpeed;
			if (maxspeed<rsubGlobalAnimHeader->m_fSpeed) 
				maxspeed=rsubGlobalAnimHeader->m_fSpeed;
		}
		return Vec2(minspeed,maxspeed); 
	}


	return MinMax; 
}

ILINE f32 Turn2BlendWeight(f32 fDesiredTurnSpeed)
{
	f32 sign = f32(sgn(-fDesiredTurnSpeed));
	f32 tb=2.37f/gf_PI;
	f32 turn_bw = fabsf( (fDesiredTurnSpeed/gf_PI)*tb ) - 0.01f;;
	//	f32 turn_bw = fabsf( fDesiredTurnSpeed/(gf_PI*0.70f) ) - 0.0f;;
	if (turn_bw> 1.0f)	turn_bw=1.0f;
	if (turn_bw< 0.0f)	turn_bw=0.00001f;
	return turn_bw*sign;
}

LMGCapabilities CAnimationSet::GetLMGPropertiesByName( const char* szAnimName, Vec2& vStrafeDirection, f32 fDesiredTurn, f32 fSlope  )
{
	LMGCapabilities lmg_caps;

	int32 nAnimID = GetAnimIDByName( szAnimName );
	if (nAnimID<0)	
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"animation-name '%s' not in CAL-file", szAnimName );
		return lmg_caps;
	}
	//check if the asset exists
	const ModelAnimationHeader* pAnim = GetModelAnimationHeader(nAnimID);
	if (pAnim==0)	
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"animation-asset '%s' not loaded", szAnimName);
		return lmg_caps;
	}


	SParametric lmg = LMG::BuildRuntimeStructLMG(this, pAnim,nAnimID);

	/*
	f32 m_turn=0;
	if (lmg.m_params[eMotionParamID_TurnSpeed].initialized)
	m_turn = 1.0f - 2.0f * lmg.m_params[eMotionParamID_TurnSpeed].blendspace.m_fAssetBlend;
	else if (lmg.m_params[eMotionParamID_TurnAngle].initialized)
	m_turn = 1.0f - 2.0f * lmg.m_params[eMotionParamID_TurnAngle].blendspace.m_fAssetBlend;
	*/

	lmg.m_BlendSpace.m_strafe	= vStrafeDirection;
	lmg.m_BlendSpace.m_turn		=	Turn2BlendWeight(fDesiredTurn);
	lmg.m_BlendSpace.m_slope	= fSlope;
	lmg.m_BlendSpace.m_speed	= 0.0f; // Not sure this is needed/used by GetLMGCaps, but it should be cheap enough to do.

	lmg.m_params[eMotionParamID_TurnSpeed].value = fDesiredTurn;

	//	float fColor[4] = {1,1,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"lmg.m_BlendSpace.m_turn: %f     m_turn: %f",lmg.m_BlendSpace.m_turn,m_turn); 
	//	g_YLine+=0x20;

	lmg_caps = LMG::GetCapabilities(this, lmg );
	return lmg_caps;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
f32 CAnimationSet::GetIWeightForSpeed(int nAnimationId, f32 Speed)
{
	Vec2 MinMax = GetMinMaxSpeedAsset_msec(nAnimationId);
	if (Speed<MinMax.x)
		return -1;
	if (Speed>MinMax.y)
		return 1;

	f32 speed=(Speed-MinMax.x);
	f32 smax=(MinMax.y-MinMax.x);
	return (speed/smax)*2-1;	
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
f32 CAnimationSet::GetDuration_sec(int nAnimationId)
{
	int32 numAnimation = (int32)m_arrAnimations.size();
	if (nAnimationId<0 || nAnimationId>=numAnimation)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index used in function GetDuration_sec'%d'", nAnimationId);
		return -1;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim==0)
		return -1;

	if (anim->m_nAssetType==CAF_File)
	{
		uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
		GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];
		f32   fDuration = rGlobalAnimHeader.m_fEndSec - rGlobalAnimHeader.m_fStartSec;
		return      fDuration;
	}

	if (anim->m_nAssetType==AIM_File)
	{
		uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
		GlobalAnimationHeaderAIM& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalAIM[GlobalAnimationID];
		f32   fDuration = rGlobalAnimHeader.m_fEndSec - rGlobalAnimHeader.m_fStartSec;
		return      fDuration;
	}

	if (anim->m_nAssetType==LMG_File)
	{
		uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
		GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[GlobalAnimationID];
		uint32 lmg = rGlobalAnimHeader.IsAssetLMG();
		assert(lmg);
		if (rGlobalAnimHeader.IsAssetLMGValid()==0)
			return 0; 

		f32 fDuration=0;
		uint32 numBS = rGlobalAnimHeader.m_arrBSAnimations.size();
		for (uint32 i=0; i<numBS; i++)
		{
			const char* aname = rGlobalAnimHeader.m_arrBSAnimations[i].m_strAnimName;
			int32 aid=GetAnimIDByName(aname);      
			assert(aid>=0);
			fDuration += GetDuration_sec(aid);
		}
		return      fDuration/numBS;
	}

	if (anim->m_nAssetType==PMG_File)
	{
		uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
		GlobalAnimationHeaderPMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalPMG[GlobalAnimationID];
		uint32 pmg = rGlobalAnimHeader.IsAssetPMG();
		assert(pmg);
		if (rGlobalAnimHeader.IsAssetPMGValid()==0)
			return 0; 

		f32 fDuration=0;
		uint32 numBS = rGlobalAnimHeader.m_arrBSAnimationsPMG.size();
		for (uint32 i=0; i<numBS; i++)
		{
			const char* aname = rGlobalAnimHeader.m_arrBSAnimationsPMG[i].m_strAnimName;
			int32 aid=GetAnimIDByName(aname);      
			assert(aid>=0);
			fDuration += GetDuration_sec(aid);
		}
		return      fDuration/numBS;
	}

	assert(0);
	return 0;
}

uint32 CAnimationSet::GetAnimationFlags(int nAnimationId)
{
	int32 numAnimation = (int32)m_arrAnimations.size();
	if (nAnimationId<0 || nAnimationId>=numAnimation)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index used in function GetAnimationFlags '%d'", nAnimationId);
		return 0;
	}
	const ModelAnimationHeader* pAnimHeader = GetModelAnimationHeader(nAnimationId);
	if (pAnimHeader==0)
		return 0;

	uint32 nGlobalAnimationID			= pAnimHeader->m_nGlobalAnimId;
	if (pAnimHeader->m_nAssetType==CAF_File)
		return g_AnimationManager.m_arrGlobalCAF[nGlobalAnimationID].m_nFlags;
	if (pAnimHeader->m_nAssetType==AIM_File)
		return g_AnimationManager.m_arrGlobalAIM[nGlobalAnimationID].m_nFlags;
	if (pAnimHeader->m_nAssetType==LMG_File)
		return g_AnimationManager.m_arrGlobalLMG[nGlobalAnimationID].m_nFlags;
	if (pAnimHeader->m_nAssetType==PMG_File)
		return g_AnimationManager.m_arrGlobalPMG[nGlobalAnimationID].m_nFlags;

	assert(0);	
	return 0;
}


uint32 CAnimationSet::GetBlendSpaceCode(int nAnimationId)
{
	int32 numAnimation = (int32)m_arrAnimations.size();
	if (nAnimationId<0 || nAnimationId>=numAnimation)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index used in function GetAnimationFlags '%d'", nAnimationId);
		return 0;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim==0)
		return 0;

	uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
	GlobalAnimationHeaderLMG& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalLMG[GlobalAnimationID];
	return rGlobalAnimHeader.m_nBlendCodeLMG;
}


//-------------------------------------------------------------------
//! Returns the given animation's start, in seconds; 0 if the id is invalid
//-------------------------------------------------------------------
f32 CAnimationSet::GetStart (int nAnimationId)
{
	int32 numAnimation = (int32)m_arrAnimations.size();
	if (nAnimationId<0 || nAnimationId>=numAnimation)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"illegal animation index used in function GetStart: '%d'", nAnimationId);
		return 0;
	}


	const ModelAnimationHeader* pAnim = &m_arrAnimations[nAnimationId];
	GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[pAnim->m_nGlobalAnimId];
	return rGAH.m_fStartSec;
}



CryAnimationPath CAnimationSet::GetAnimationPath(const char* szAnimationName) 
{
	CryAnimationPath p;
	p.m_key0.SetIdentity();
	p.m_key1.SetIdentity();
	int AnimID = GetAnimIDByName(szAnimationName);
	if (AnimID<0)
		return p;

	const ModelAnimationHeader* anim = GetModelAnimationHeader(AnimID);
	if (anim)
	{
		uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
		GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];
		if ( rGlobalAnimHeader.IsAssetLMG() )
			return p;

		//if ( rGlobalAnimHeader.IsAssetOnDemand() && !rGlobalAnimHeader.IsAssetCreated() )
		//	return p;

		if (!rGlobalAnimHeader.IsAssetCreated())
			return p;

		const CModelJoint* pModelJoint = &m_pModel->m_ModelSkeleton.m_arrModelJoints[0];
		IController* pController = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID].GetControllerByJointCRC32(pModelJoint[0].m_nJointCRC32);//pController = pModelJoint[0].m_arrControllersMJoint[AnimID];
		// TODO: for locoman groups, pController is NULL (what to do here!!)
		if (pController)
		{
			Diag33 scale;
			f32 fKeyTime0=g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID].NTime2KTime(0);
			pController->GetOPS( fKeyTime0, p.m_key0.q, p.m_key0.t,scale);	
			f32 fKeyTime1=g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID].NTime2KTime(1);
			pController->GetOPS( fKeyTime1, p.m_key1.q, p.m_key1.t,scale);	
		}
	}
	return p;
};

const QuatT& CAnimationSet::GetAnimationStartLocation(const char* szAnimationName) 
{
	static QuatT DefaultStartLocation; 
	DefaultStartLocation.SetIdentity();

	int32 AnimID = GetAnimIDByName(szAnimationName);
	if (AnimID<0)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"animation-name not in cal-file: %s",szAnimationName);
		return DefaultStartLocation;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(AnimID);
	if (anim==0)
		return DefaultStartLocation;

	uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];
	return  rGlobalAnimHeader.m_StartLocation;
};

const QuatT& CAnimationSet::GetJointStartLocation(const char* szAnimationName, int32 jointCRC32) 
{	
	static QuatT DefaultStartLocation; 
	DefaultStartLocation.SetIdentity();

	int32 AnimID = GetAnimIDByName(szAnimationName);
	if (AnimID<0)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"animation-name not in cal-file: %s",szAnimationName);
		return DefaultStartLocation;
	}

	const ModelAnimationHeader* anim = GetModelAnimationHeader(AnimID);
	if (anim==0)
		return DefaultStartLocation;

	uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];
	IController* pController = rGlobalAnimHeader.GetControllerByJointCRC32(jointCRC32);
	if (pController==0)
		return DefaultStartLocation;
	pController->GetOP( rGlobalAnimHeader.NTime2KTime(0), DefaultStartLocation.q, DefaultStartLocation.t);

	return DefaultStartLocation;
};

const char* CAnimationSet::GetFilePathByName (const char* szAnimationName)
{
	int32 AnimID = GetAnimIDByName(szAnimationName);
	if (AnimID<0)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"animation-name not in cal-file: %s",szAnimationName);
		return 0;
	}
	const ModelAnimationHeader* anim = GetModelAnimationHeader(AnimID);
	if (anim==0)
		return 0;

	uint32 GlobalAnimationID			= anim->m_nGlobalAnimId;
	if (anim->m_nAssetType==CAF_File)
		return g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID].GetFilePath();
	if (anim->m_nAssetType==AIM_File)
		return g_AnimationManager.m_arrGlobalAIM[GlobalAnimationID].GetFilePath();
	if (anim->m_nAssetType==LMG_File)
		return g_AnimationManager.m_arrGlobalLMG[GlobalAnimationID].GetFilePath();
	if (anim->m_nAssetType==PMG_File)
		return g_AnimationManager.m_arrGlobalPMG[GlobalAnimationID].GetFilePath();

	assert(0);
	return  0;
};

const char* CAnimationSet::GetFilePathByID(int nAnimationId)
{
	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim==0)
		return 0;

	uint32 GlobalAnimationID			= anim->m_nGlobalAnimId;
	if (anim->m_nAssetType==CAF_File)
		return g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID].GetFilePath();
	if (anim->m_nAssetType==AIM_File)
		return g_AnimationManager.m_arrGlobalAIM[GlobalAnimationID].GetFilePath();
	if (anim->m_nAssetType==LMG_File)
		return g_AnimationManager.m_arrGlobalLMG[GlobalAnimationID].GetFilePath();
	if (anim->m_nAssetType==PMG_File)
		return g_AnimationManager.m_arrGlobalPMG[GlobalAnimationID].GetFilePath();

	assert(0);
	return  0;
};





int32 CAnimationSet::GetGlobalIDByName(const char* szAnimationName)
{
	int32 AnimID = GetAnimIDByName(szAnimationName);
	if (AnimID<0)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"animation-name not in cal-file: %s",szAnimationName);
		return -1;
	}
	const ModelAnimationHeader* anim = GetModelAnimationHeader(AnimID);
	if (anim==0)
		return -1;

	return (int32)anim->m_nGlobalAnimId;
}
int32 CAnimationSet::GetGlobalIDByAnimID(int nAnimationId)
{
	const ModelAnimationHeader* anim = GetModelAnimationHeader(nAnimationId);
	if (anim==0)
		return -1;
	return (int32)anim->m_nGlobalAnimId;
}


//if the return-value is positive then the closest-quaternion is at key[0...1]
f32 CAnimationSet::GetClosestQuatInChannel(const char* szAnimationName,int32 JointID, const Quat& reference)
{
	int32 numJoints = m_pModel->m_ModelSkeleton.m_arrModelJoints.size();
	if (JointID<0 || JointID>=numJoints)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"illegal joint index: %d",JointID);
		return -1; // wrong animation name
	}

	int32 AnimID = GetAnimIDByName(szAnimationName);
	if (AnimID<0)
	{
		if (Console::GetInst().ca_AnimWarningLevel>0)
			AnimFileWarning(m_pModel->GetModelFilePath(),"animation-name not in cal-file: %s",szAnimationName);
		return -1; // wrong animation name
	}
	const ModelAnimationHeader* anim = GetModelAnimationHeader(AnimID);
	if (anim==0)
		return -1; //asset does not exist

	f32 smalest_dot=0.0f;
	f32 closest_key=-1;

	uint32 GlobalAnimationID = anim->m_nGlobalAnimId;
	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID];
	f32	fDuration = rGlobalAnimHeader.m_fEndSec - rGlobalAnimHeader.m_fStartSec;
	f32 timestep = (1.0f/30.0f)/fDuration;


	IController* pController = g_AnimationManager.m_arrGlobalCAF[GlobalAnimationID].GetControllerByJointCRC32(m_pModel->m_ModelSkeleton.m_arrModelJoints[JointID].m_nJointCRC32);//m_pModel->m_ModelSkeleton.m_arrModelJoints[JointID].m_arrControllersMJoint[AnimID];
	if (pController)
	{
		for (f32 t=0; t<1.0f; t=t+timestep)
		{
			//NOTE: all quternions in the channel are relative to the parent, which means they are usually close to identity

			Quat keyquat;	pController->GetO( rGlobalAnimHeader.NTime2KTime(t), keyquat);	


			f32 cosine=fabsf(keyquat|reference);
			if (smalest_dot<cosine)
			{
				smalest_dot=cosine;
				closest_key=t;
			}
		}
	}

	return closest_key*fDuration;
}

