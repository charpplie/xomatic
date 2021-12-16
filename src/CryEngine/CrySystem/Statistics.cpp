#include <StdAfx.h>

#include <ISystem.h>
#include <I3DEngine.h>
#include <IRenderer.h>
#include <IConsole.h>
#include <ICryAnimation.h>
#include <IPerfHud.h>
#include <CryPath.h>
#include <CrySizer.h>
#include "CrySizerImpl.h"
#include "CrySizerStats.h"
#include "System.h"
#include "CryMemoryManager.h"
#include <ITestSystem.h>
#include <IScriptSystem.h>
#include <ISound.h>
#include <IAudioDevice.h> // Iteration through Wavebanks
#include <ResourceCompilerHelper.h>			// CResourceCompilerHelper
#include "LoadingProfiler.h"

// Access to some game info.
#include "IGame.h"							// IGame
#include "IGameFramework.h"		// IGameFramework
#include "../CryAction/ILevelSystem.h"		// IGameFramework

#ifdef WIN32
	#include "Psapi.h"
	typedef BOOL (WINAPI *GetProcessMemoryInfoProc)( HANDLE,PPROCESS_MEMORY_COUNTERS,DWORD );
#endif

#if defined(PS3)
	#include <CryMTrace.h>
	#include <cell/fs/cell_fs_file_api.h>
#else

	#if defined(WIN32) || defined(XENON)
	#pragma pack(push,1)
	const struct PEHeader_DLL
	{
		DWORD signature;
		IMAGE_FILE_HEADER _head;
		IMAGE_OPTIONAL_HEADER opt_head;
		IMAGE_SECTION_HEADER *section_header;  // actual number in NumberOfSections
	};
	#pragma pack(pop)
	#endif 
#endif//PS3

extern int CryMemoryGetAllocatedSize();
static void SaveLevelStats( IConsoleCmdArgs *pArgs );

#if defined(XENON)
#if defined (_RELEASE) || defined(SAVE_SAVELEVELSTATS_IN_ROOT)
	#define g_szTestResults "e:\\TestResults"
#else
	#define g_szTestResults "d:\\TestResults"
#endif
#else
	#define g_szTestResults "TestResults"
#endif

class CResourceCollector :public IResourceCollector
{	
	struct SInstanceEntry
	{
//		AABB			m_AABB;
		uint32		m_dwFileNameId;				// use with m_FilenameToId,  m_IdToFilename
	};

	struct SAssetEntry
	{
		SAssetEntry() :m_dwInstanceCnt(0), m_dwDependencyCnt(0), m_dwMemSize(0xffffffff), m_dwFileSize(0xffffffff)
		{
		}

		string		m_sFileName;
		uint32		m_dwInstanceCnt;			// 1=this asset is used only once in the level, 2, 3, ...
		uint32		m_dwDependencyCnt;		// 1=this asset is only used by one asset, 2, 3, ...
		uint32		m_dwMemSize;					// 0xffffffff if unknown (only needed to verify disk file size)
		uint32		m_dwFileSize;					// 0xffffffff if unknown
	};

public:	// -----------------------------------------------------------------------------

	CResourceCollector()
	{
		m_bEnabled = false;
	}
	void Enable( bool bEnabled ) { m_bEnabled = bEnabled; }

	// compute m_dwDependencyCnt
	void ComputeDependencyCnt()
	{
		std::set<SDependencyPair>::const_iterator it, end=m_Dependencies.end();

		for(it=m_Dependencies.begin();it!=end;++it)
		{
			const SDependencyPair &rRef = *it;

			++m_Assets[rRef.m_idDependsOnAsset].m_dwDependencyCnt;
		}
	}

	// watch out: this function modifies internal data
	void LogData( ILog &rLog )
	{
		if (!m_bEnabled)
			return;

		rLog.Log(" ");

		{
			rLog.Log("Assets:");

			std::vector<SAssetEntry>::const_iterator it, end=m_Assets.end();
			uint32 dwAssetID=0;

			for(it=m_Assets.begin();it!=end;++it,++dwAssetID)
			{
				const SAssetEntry &rRef = *it;

				rLog.Log(" A%d: inst:%5d dep:%d mem:%9d file:%9d name:%s",dwAssetID,rRef.m_dwInstanceCnt,rRef.m_dwDependencyCnt,rRef.m_dwMemSize,rRef.m_dwFileSize,rRef.m_sFileName.c_str());
			}
		}

		rLog.Log(" ");

		{
			rLog.Log("Dependencies:");

			std::set<SDependencyPair>::const_iterator it, end=m_Dependencies.end();

			uint32 dwCurrentAssetID=0xffffffff;
			uint32 dwSumFile=0;

			for(it=m_Dependencies.begin();it!=end;++it)
			{
				const SDependencyPair &rRef = *it;		

				if(rRef.m_idAsset!=dwCurrentAssetID)
				{
					if(dwSumFile!=0 && dwSumFile!=0xffffffff)
						rLog.Log("                                                ---> sum file: %d KB",(dwSumFile+1023)/1024);

					dwSumFile=0;

					rLog.Log(" ");
					rLog.Log(" A%d '%s' depends on", rRef.m_idAsset,m_Assets[rRef.m_idAsset].m_sFileName.c_str());
				}

				uint32 dwFileSize = m_Assets[rRef.m_idDependsOnAsset].m_dwFileSize;

				rLog.Log("        A%d file:%9d dep:%d '%s'",rRef.m_idDependsOnAsset,dwFileSize,m_Assets[rRef.m_idDependsOnAsset].m_dwDependencyCnt,m_Assets[rRef.m_idDependsOnAsset].m_sFileName.c_str());

				if(dwFileSize!=0xffffffff)
					dwSumFile += dwFileSize;

				dwCurrentAssetID=rRef.m_idAsset;
			}

			if(dwSumFile!=0 && dwSumFile!=0xffffffff)
				rLog.Log("                                                ---> sum file: %d KB",(dwSumFile+1023)/1024);
		}

		rLog.Log(" ");

		{
			rLog.Log("SourceAtoms:");

			std::set<SDependencyPair>::const_iterator it;

			while(!m_Dependencies.empty())
			{
				for(it=m_Dependencies.begin();it!=m_Dependencies.end();++it)
				{
					const SDependencyPair &rRef1 = *it;

					rLog.Log(" ");
					std::set<uint32> localDependencies;

					localDependencies.insert(rRef1.m_idAsset);

					RecursiveMove(rRef1.m_idAsset,localDependencies);

					PrintDependencySet(rLog,localDependencies);
					break;
				}
			}
		}
	}

	// interface IResourceCollector -------------------------------------------------

	VIRTUAL bool AddResource( const char *szFileName, const uint32 dwSize=0xffffffff )
	{
		if (!m_bEnabled)
			return true;

		uint32 dwNewAssetIdOrInvalid = _AddResource(szFileName,dwSize);

		return dwNewAssetIdOrInvalid!=0xffffffff;
	}

	VIRTUAL void AddInstance( const char *_szFileName, void *pInstance )
	{
		if (!m_bEnabled)
			return;

		assert(pInstance);

		{
			std::set<void *>::const_iterator itInstance = m_ReportedInstances.find(pInstance);

			if(itInstance!=m_ReportedInstances.end())
				return;
		}

		string sOutputFileName = UnifyFilename(_szFileName);

		std::map<string,uint32>::const_iterator it = m_FilenameToId.find(sOutputFileName);

		if(it==m_FilenameToId.end())
		{
			OutputDebugString("ERROR: file wasn't registered with AddResource(): '");
			OutputDebugString(sOutputFileName.c_str());
			OutputDebugString("'\n");
			assert(0);		// asset wasn't registered yet AddResource() missing - unpredictable result might happen
			return;
		}

		uint32 dwAssetId = it->second;
/*
		// debug
		char str[256];
		sprintf(str,"AddInstance: %p '",pInstance);
		OutputDebugString(str);
		OutputDebugString(sOutputFileName.c_str());
		OutputDebugString("'\n");
*/
		++m_Assets[dwAssetId].m_dwInstanceCnt;
		m_ReportedInstances.insert(pInstance);
	}

	VIRTUAL void OpenDependencies( const char *_szFileName )
	{
		if (!m_bEnabled)
			return;

		string sOutputFileName = UnifyFilename(_szFileName);

		std::map<string,uint32>::const_iterator it = m_FilenameToId.find(sOutputFileName);


		if(it==m_FilenameToId.end())
		{
			m_OpenedAssetId.push_back(0xffffffff);			// CloseDependencies() relies on that

			OutputDebugString("ERROR: file wasn't registered with AddResource(): '");
			OutputDebugString(sOutputFileName.c_str());
			OutputDebugString("'\n");
			//assert(0);		// asset wasn't registered yet AddResource() missing - unpredictable result might happen
			return;
		}

		uint32 dwAssetId = it->second;

		m_OpenedAssetId.push_back(dwAssetId);
	}

	VIRTUAL void Reset()
	{
		m_Assets.resize(0);
		m_Dependencies.clear();
		m_FilenameToId.clear();
		m_OpenedAssetId.resize(0);
		m_ReportedInstances.clear();
		m_ResourceEntries.resize(0);
	}

	VIRTUAL void CloseDependencies()
	{
		if (!m_bEnabled)
			return;

		assert(!m_OpenedAssetId.empty());		// internal error - OpenDependencies() should match CloseDependencies()

		m_OpenedAssetId.pop_back();
	}

private: // -----------------------------------------------------------------------

	struct SDependencyPair
	{
		SDependencyPair( const uint32 idAsset, const uint32 idDependsOnAsset ) :m_idAsset(idAsset), m_idDependsOnAsset(idDependsOnAsset)
		{
		}

		uint32			m_idAsset;							// AssetID
		uint32			m_idDependsOnAsset;			// AssetID

		bool operator<( const SDependencyPair &rhs ) const
		{
			if(m_idAsset<rhs.m_idAsset) return true;
			if(m_idAsset>rhs.m_idAsset) return false;

			return m_idDependsOnAsset<rhs.m_idDependsOnAsset;
		}
	};

	std::vector<uint32>							m_OpenedAssetId;			// to track for dependencies
	std::map<string,uint32>					m_FilenameToId;				// could be done more efficiently
	std::vector<SAssetEntry>				m_Assets;							// could be done more efficiently
	std::vector<SInstanceEntry>			m_ResourceEntries;		//
	std::set<SDependencyPair>				m_Dependencies;				//
	std::set<void *>								m_ReportedInstances;	// to avoid counting them twice
	bool                            m_bEnabled;

	// ---------------------------------------------------------------------

	string UnifyFilename( const char *_szFileName ) const
	{
		char *szFileName=(char *)_szFileName;

		// as bump and normal maps become combined during loading e.g.  blah.tif+blah_ddn.dds
		// the filename needs to be adjusted
		{
			char *pSearchForPlus = szFileName;

			while(*pSearchForPlus!=0 && *pSearchForPlus!='+')
				++pSearchForPlus;

			if(*pSearchForPlus=='+')
				szFileName=pSearchForPlus+1;
		}

		string sOutputFileName = CResourceCompilerHelper::GetOutputFilename(szFileName);

		sOutputFileName = PathUtil::ToUnixPath(sOutputFileName);
		sOutputFileName.MakeLower();

		return sOutputFileName;
	}

	// Returns:
	//   0xffffffff if asset was already known (m_dwInstanceCnt will be increased), AssetId otherwise
	uint32 _AddResource( const char *_szFileName, const uint32 dwSize=0xffffffff )
	{
		assert(_szFileName);

		if(_szFileName[0]==0)
			return 0xffffffff;								// no name provided - ignore this case - this often means the feature is not used

		uint32 dwNewAssetIdOrInvalid=0xffffffff;

		string sOutputFileName = UnifyFilename(_szFileName);

		std::map<string,uint32>::const_iterator it = m_FilenameToId.find(sOutputFileName);
		uint32 dwAssetId;

		if(it!=m_FilenameToId.end())
			dwAssetId = it->second;
		else
		{
			dwAssetId = m_FilenameToId.size();
			m_FilenameToId[sOutputFileName] = dwAssetId;

			SAssetEntry NewAsset;

			NewAsset.m_sFileName=sOutputFileName;

			//			if(dwSize==0xffffffff)
			{
				CCryFile file;

				if(file.Open(sOutputFileName.c_str(),"rb"))
					NewAsset.m_dwFileSize = file.GetLength();
			}

			dwNewAssetIdOrInvalid = dwAssetId;
			m_Assets.push_back(NewAsset);
		}

		SAssetEntry &rAsset = m_Assets[dwAssetId];

		if(dwSize!=0xffffffff)										// if size was specified
		{
			if(rAsset.m_dwMemSize==0xffffffff)
			{
				rAsset.m_dwMemSize=dwSize;						// store size
			}
			else
				assert(rAsset.m_dwMemSize==dwSize);		// size should always be the same
		}

//		rAsset.m_dwInstanceCnt+=dwInstanceCount;

		// debugging
//		char str[1204];
//		sprintf(str,"_AddResource %s(size=%d cnt=%d)\n",_szFileName,rAsset.m_dwInstanceCnt,rAsset.m_dwMemSize);
//		OutputDebugString(str);


		SInstanceEntry instance;

		instance.m_dwFileNameId=dwAssetId;

		AddDependencies(dwAssetId);

		m_ResourceEntries.push_back(instance);
		return dwNewAssetIdOrInvalid;
	}

	void AddDependencies( const uint32 dwPushedAssetId )
	{
		std::vector<uint32>::const_iterator it, end=m_OpenedAssetId.end();

		for(it=m_OpenedAssetId.begin();it!=end;++it)
		{
			uint32 dwOpendedAssetId = *it;

			if(dwOpendedAssetId==0xffffffff)
				continue;		//  asset wasn't registered yet AddResource() missing

			m_Dependencies.insert(SDependencyPair(dwOpendedAssetId,dwPushedAssetId));
		}
	}

	void PrintDependencySet( ILog &rLog, const std::set<uint32> &Dep )
	{
		rLog.Log("      {");
		std::set<uint32>::const_iterator it, end=Dep.end();
		uint32 dwSumFile=0;

		// iteration could be optimized
		for(it=Dep.begin();it!=end;++it)
		{
			uint32 idAsset = *it;

			uint32 dwFileSize = m_Assets[idAsset].m_dwFileSize;

			if(dwFileSize!=0xffffffff)
				dwSumFile += dwFileSize;

			rLog.Log("        A%d file:%9d dep:%d '%s'",idAsset,m_Assets[idAsset].m_dwFileSize,m_Assets[idAsset].m_dwDependencyCnt,m_Assets[idAsset].m_sFileName.c_str());
		}
		rLog.Log("      }                                           ---> sum file: %d KB",(dwSumFile+1023)/1024);
	}

	// find all dependencies to it and move to localDependencies
	void RecursiveMove( const uint32 dwCurrentAssetID, std::set<uint32> &localDependencies )
	{
		bool bProcess=true;

		// iteration could be optimized
		while(bProcess)
		{
			bProcess=false;

			std::set<SDependencyPair>::iterator it;

			for(it=m_Dependencies.begin();it!=m_Dependencies.end();++it)
			{
				SDependencyPair Pair = *it;

				if(Pair.m_idAsset==dwCurrentAssetID || Pair.m_idDependsOnAsset==dwCurrentAssetID)
				{
					uint32 idAsset = (Pair.m_idAsset==dwCurrentAssetID) ? Pair.m_idDependsOnAsset : Pair.m_idAsset;

					localDependencies.insert(idAsset);

					m_Dependencies.erase(it);

					RecursiveMove(idAsset,localDependencies);
					bProcess=true;
					break;
				}
			}
		}
	}

	friend class CStatsToExcelExporter;
};

#define MAX_LODS 6

//////////////////////////////////////////////////////////////////////////
// Statistics about currently loaded level.
//////////////////////////////////////////////////////////////////////////
struct SCryEngineStats
{
	struct StatObjInfo
	{
		int nVertices;
		int nIndices;
		int nIndicesPerLod[MAX_LODS];
		int nMeshSize;
		int nMeshSizeLoaded;
		int nTextureSize;
		int nPhysProxySize;
		int nPhysPrimitives;
		int nLods;
		int nSubMeshCount;
		int nNumRefs;
		bool bSplitLods;
		IStatObj *pStatObj;
	};
	struct CharacterInfo
	{
		CharacterInfo() :nVertices(0), nIndices(0), nMeshSize(0), nTextureSize(0), nLods(0), nInstances(0), nPhysProxySize(0), pModel(0)
		{
		}
		
		int nVertices;
		int nIndices;
		int nVerticesPerLod[MAX_LODS];
		int nIndicesPerLod[MAX_LODS];
		int nMeshSize;
		int nTextureSize;
		int nLods;
		int nInstances;
		int nPhysProxySize;
		ICharacterModel *pModel;
	};
	struct MeshInfo
	{
		int nVerticesSum;
		int nIndicesSum;
		int nCount;
		int nMeshSizeDev;
    int nMeshSizeSys;
		int nTextureSize;
		const char *name;
	};
	struct MemInfo : public SCryEngineStatsGlobalMemInfo
	{
		MemInfo() : m_pSizer(0), m_pStats(0) {}
		~MemInfo() {SAFE_DELETE(m_pSizer); SAFE_DELETE(m_pStats); }

		//int totalUsedInModules;
		//int totalCodeAndStatic;
		//int countedMemoryModules;
		//uint64 totalAllocatedInModules;
		//int totalNumAllocsInModules;
		CrySizerImpl* m_pSizer;
		CrySizerStats* m_pStats;

		//std::vector<SCryEngineStatsModuleInfo> modules;
	};

	struct ProfilerInfo
	{
		string m_name;
		string m_module;

		//! Total time spent in this counter including time of child profilers in current frame.
		float m_totalTime;
		//! Self frame time spent only in this counter (But includes recursive calls to same counter) in current frame.
		int64 m_selfTime;
		//! How many times this profiler counter was executed.
		int m_count;
		//! Total time spent in this counter during all profiling period.
		int64 m_sumTotalTime;
		//! Total self time spent in this counter during all profiling period.
		int64 m_sumSelfTime;
		//! Displayed quantity (interpolated or avarage).
		float m_displayedValue;
		//! Displayed quantity (current frame value).
//		float m_displayedCurrentValue;
		//! How variant this value.
		float m_variance;
		// min value
		float m_min;
		// max value
		float m_max;

		int m_mincount;

		int m_maxcount;

	};

	struct SPeakProfilerInfo
	{
		ProfilerInfo profiler;
		float peakValue;
		float avarageValue;
		float variance;
		int pageFaults; // Number of page faults at this frame.
		int count;  // Number of times called for peak.
		float when; // when it added.
	};

	struct SModuleProfilerInfo
	{
		string name;
		float	overBugetRatio;
	};


	SCryEngineStats()
		: nSummary_CodeAndStaticSize(0)

		, nSummaryCharactersSize(0)
		
		//, nSummary_TextureSize(0)
		, nSummary_UserTextureSize(0)
		, nSummary_EngineTextureSize(0)
		, nSummary_TexturesStreamingThroughput(0.0f)

		, nStatObj_SummaryTextureSize(0)
		, nStatObj_SummaryMeshSize(0)
		, nStatObj_TotalCount(0)

		, nChar_SummaryMeshSize(0)
		, nChar_SummaryTextureSize(0)
		, nChar_NumInstances(0)

		, fLevelLoadTime(0.0f)
		, nSummary_TexturesPoolSize(0)
	{
		ISystem *pSystem = GetISystem();
		I3DEngine *p3DEngine = pSystem->GetI3DEngine();
		IRenderer *pRenderer = pSystem->GetIRenderer();

		nTotalAllocatedMemory = CryMemoryGetAllocatedSize();
		nSummaryMeshSize =  *((int*)pRenderer->EF_Query(EFQ_Alloc_Mesh_SysMem));
		nSummaryMeshCount =  *((int*)pRenderer->EF_Query(EFQ_Mesh_Count));
		nAPI_MeshSize =  *((int*)pRenderer->EF_Query(EFQ_Alloc_APIMesh));
		nSummaryScriptSize = pSystem->GetIScriptSystem()->GetScriptAllocSize();

		IMemoryManager::SProcessMemInfo procMeminfo;
		GetISystem()->GetIMemoryManager()->GetProcessMemInfo( procMeminfo );

		nWin32_WorkingSet = procMeminfo.WorkingSetSize;
		nWin32_PeakWorkingSet = procMeminfo.PeakWorkingSetSize;
		nWin32_PagefileUsage = procMeminfo.PagefileUsage;
		nWin32_PeakPagefileUsage = procMeminfo.PeakPagefileUsage;
		nWin32_PageFaultCount = procMeminfo.PageFaultCount;

		if (gEnv->pGame)
			fLevelLoadTime = gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetLastLevelLoadTime();

		if (p3DEngine) {
			p3DEngine->FillDebugFPSInfo(infoFPS);
		}
	}

	uint64 nWin32_WorkingSet;
	uint64 nWin32_PeakWorkingSet;
	uint64 nWin32_PagefileUsage;
	uint64 nWin32_PeakPagefileUsage;
	uint64 nWin32_PageFaultCount;

	uint32 nTotalAllocatedMemory;
	uint32 nSummary_CodeAndStaticSize;  // Total size of all code plus static data

	uint32 nSummaryScriptSize;
	uint32 nSummaryCharactersSize;
	uint32 nSummaryMeshCount;
	uint32 nSummaryMeshSize;
	
	uint32 nAPI_MeshSize;    // Allocated by DirectX
	

	uint32 nSummary_TextureSize;       // Total size of all textures
	uint32 nSummary_UserTextureSize;   // Size of eser textures, (from files...)
	uint32 nSummary_EngineTextureSize; // Dynamic Textures
	uint32 nSummary_TexturesPoolSize; // Dynamic Textures
	float nSummary_TexturesStreamingThroughput; // in KB/sec

	uint32 nStatObj_SummaryTextureSize;
	uint32 nStatObj_SummaryMeshSize;
	uint32 nStatObj_TotalCount; // Including sub-objects.

	uint32 nChar_SummaryMeshSize;
	uint32 nChar_SummaryTextureSize;
	uint32 nChar_NumInstances;

	float fLevelLoadTime;
	SDebugFPSInfo infoFPS;


	std::vector<StatObjInfo> objects;
	std::vector<CharacterInfo> characters;
	std::vector<ITexture*> textues;
	std::vector<MeshInfo> meshes;
	std::vector<IMaterial*> materials;
	std::vector<IWavebank*> wavebanks;
	std::vector<ISoundProfileInfo*> soundinfos;
	std::vector<ProfilerInfo> profilers;
	std::vector<SPeakProfilerInfo> peaks;
	std::vector<SModuleProfilerInfo> moduleprofilers;
	std::vector<SAnimationStatistics> animations;
	std::vector<SLoadingProfilerInfo> loading;

	MemInfo memInfo;
};

inline bool CompareFrameProfilersValueStats( const SCryEngineStats::ProfilerInfo &p1,const SCryEngineStats::ProfilerInfo &p2 )
{
	return p1.m_displayedValue > p2.m_displayedValue;
}

class CEngineStats
{
	CEngineStats( bool bDepends )
	{
		m_ResourceCollector.Enable(bDepends);
		Collect();
	}

private: // ----------------------------------------------------------------------------

	// Collect all stats.
	void Collect();

	void CollectGeometry();
	void CollectCharacters();
	void CollectMaterialDependencies();
	void CollectTextures();
  void CollectMaterials();
	void CollectVoxels();
	void CollectRenderMeshes();
	void CollectWavebanks();
	void CollectSoundInfos();
	void CollectEntityDependencies();
	void CollectMemInfo();
	void CollectProfileStatistics();
	void CollectAnimations();
	void CollectLoadingData();

	// Arguments:
	//   pObj - 0 is ignored
	//   pMat - 0 if IStatObjet Material should be used
	void AddResource_StatObjWithLODs( IStatObj *pObj, CrySizerImpl &statObjTextureSizer, IMaterial *pMat=0 );
	//
	void AddResource_SingleStatObj( IStatObj &rData );
	//
	void AddResource_CharInstance( ICharacterInstance &rData );
	//
	void AddResource_Material( IMaterial &rData, const bool bSubMaterial=false );


	CResourceCollector		m_ResourceCollector;		// dependencies between assets 
	SCryEngineStats				m_stats;								//

	friend void SaveLevelStats( IConsoleCmdArgs *pArgs );
};

//////////////////////////////////////////////////////////////////////////
void CEngineStats::Collect()
{
	//////////////////////////////////////////////////////////////////////////
	// Collect CGFs
	//////////////////////////////////////////////////////////////////////////
	CollectMemInfo(); // First of all collect memory info for modules (must be first).
	CollectGeometry();
	CollectCharacters();
	CollectTextures();
  CollectMaterials();
	CollectRenderMeshes();
	CollectWavebanks();
	CollectSoundInfos();
	CollectMaterialDependencies();
	CollectEntityDependencies();
	CollectProfileStatistics();
	CollectAnimations();
	CollectLoadingData();
}

inline bool CompareMaterialsByName( IMaterial *pMat1,IMaterial *pMat2 )
{
  return pMat1->GetName() > pMat2->GetName();
}

inline bool CompareTexturesBySizeFunc( ITexture *pTex1,ITexture *pTex2 )
{
	return pTex1->GetDataSize() > pTex2->GetDataSize();
}
inline bool CompareStatObjBySizeFunc( const SCryEngineStats::StatObjInfo &s1,const SCryEngineStats::StatObjInfo &s2 )
{
	return (s1.nMeshSize+s1.nTextureSize) > (s2.nMeshSize+s2.nTextureSize);
}
inline bool CompareCharactersBySizeFunc( const SCryEngineStats::CharacterInfo &s1,const SCryEngineStats::CharacterInfo &s2 )
{
	return (s1.nMeshSize+s1.nTextureSize) > (s2.nMeshSize+s2.nTextureSize);
}
inline bool CompareWavebanksBySizeFunc( IWavebank *pWB1,IWavebank *pWB2 )
{
	return pWB1->GetInfo()->nFileSize > pWB2->GetInfo()->nFileSize;
}
inline bool CompareSoundInfosByTimesPlayedFunc( ISoundProfileInfo *pSI1, ISoundProfileInfo *pSI2 )
{
	return pSI1->GetInfo()->nTimesPlayed > pSI2->GetInfo()->nTimesPlayed;
}
inline bool CompareRenderMeshByTypeName( IRenderMesh *pRM1,IRenderMesh *pRM2 )
{
	return strcmp( pRM1->GetTypeName(),pRM2->GetTypeName() ) < 0;
}


void CEngineStats::AddResource_SingleStatObj( IStatObj &rData )
{
	if(!m_ResourceCollector.AddResource(rData.GetFilePath()))
		return;		// was already registered

	// dependencies

	if(rData.GetMaterial())
	{
		m_ResourceCollector.OpenDependencies(rData.GetFilePath());

		AddResource_Material(*rData.GetMaterial());

		m_ResourceCollector.CloseDependencies();
	}
}

void CEngineStats::AddResource_CharInstance( ICharacterInstance &rData )
{
	IMaterial *pMat = rData.GetMaterial();

	if(!m_ResourceCollector.AddResource(rData.GetFilePath()))
		return;		// was already registered

	// dependencies

	if(rData.GetMaterial())
	{
		m_ResourceCollector.OpenDependencies(rData.GetFilePath());

		AddResource_Material(*rData.GetMaterial());

		m_ResourceCollector.CloseDependencies();
	}
}


void CEngineStats::AddResource_Material( IMaterial &rData, const bool bSubMaterial )
{
	if(!bSubMaterial)
	{
		string sName = string(rData.GetName())+".mtl";

		if(!m_ResourceCollector.AddResource(sName))
			return; // was already registered

		// dependencies

		m_ResourceCollector.OpenDependencies(sName);
	}

	{
		SShaderItem &rItem = rData.GetShaderItem();

		uint32 dwSubMatCount = rData.GetSubMtlCount();

		for(uint32 dwSubMat=0;dwSubMat<dwSubMatCount;++dwSubMat)
		{
			IMaterial *pSub = rData.GetSubMtl(dwSubMat);

			if(pSub)
				AddResource_Material(*pSub,true);		
		}

		// this material
		if(rItem.m_pShaderResources)
			for(uint32 dwI=0;dwI<EFTT_MAX;++dwI)
			{
				SEfResTexture *pTex = rItem.m_pShaderResources->GetTexture(dwI);

				if(!pTex)
					continue;

				uint32 dwSize=0xffffffff;
				/*
				if(pTex->m_Sampler.m_pITex)
				{
				dwSize = pTex->m_Sampler.m_pITex->GetDataSize();

				assert(pTex->m_Name);
				assert(pTex->m_Sampler.m_pITex->GetName());

				string sTex = PathUtil::ToUnixPath(PathUtil::ReplaceExtension(pTex->m_Name,""));
				string sSampler = PathUtil::ToUnixPath(PathUtil::ReplaceExtension(pTex->m_Sampler.m_pITex->GetName(),""));

				if(stricmp(sTex.c_str(),sSampler.c_str())!=0)
				{
				char str[1024];

				sprintf(str,"IGNORE  '%s' '%s'\n",sTex.c_str(),sSampler.c_str());
				OutputDebugString(str);
				dwSize=0;
				//				IGNORE  'Textures/gradf' 'Editor/Objects/gradf'
				//				IGNORE  'textures/cubemaps/auto_cubemap' '$RT_CM'
				//				IGNORE  '' 'Textures/Defaults/White_ddn'
				//				IGNORE  '' 'textures/defaults/oceanwaves_ddn'
				//				IGNORE  '' 'textures/sprites/fire_blur1_ddn'
				//				IGNORE  '' 'textures/sprites/fire_blur1_ddn'
				//				IGNORE  '' 'Game/Objects/Library/Barriers/Sandbags/sandbags_ddn'
				//				IGNORE  '' 'objects/characters/human/us/nanosuit/nanosuit_ddn'
				//				IGNORE  '' 'objects/characters/human/us/nanosuit/nanosuit_ddndif'
				}

				//		if(dwI==EFTT_NORMALMAP && rItem.m_pShaderResources->m_Textures[EFTT_BUMP])
				//			dwSize=0;			// if there is a _bump texture, we don't make use of the ddn any more, so not space is wasted for this but we still need to report the file

				m_ResourceCollector.AddResource(pTex->m_Sampler.m_pITex->GetName(),dwSize);		// used texture
				}
				else
				*/			
				//		CryLog("AddResource ITex (%d): '%s' '%s'",dwI,pTex->m_Name.c_str(),pTex->m_Sampler.m_pITex->GetName());

				if(pTex->m_Sampler.m_pITex)
					m_ResourceCollector.AddResource(pTex->m_Sampler.m_pITex->GetName(),dwSize);
				//		m_ResourceCollector.AddResource(pTex->m_Name,dwSize);
			}
	}


	if(!bSubMaterial)
		m_ResourceCollector.CloseDependencies();
}


void CEngineStats::AddResource_StatObjWithLODs( IStatObj *pObj, CrySizerImpl &statObjTextureSizer, IMaterial *pMat )
{
	if(!pObj)
		return;

	SCryEngineStats::StatObjInfo si;

	si.pStatObj = pObj;

	memset(si.nIndicesPerLod,0,sizeof(si.nIndicesPerLod));

	// dependencies
	AddResource_SingleStatObj(*si.pStatObj);

	CrySizerImpl localTextureSizer;

	si.nLods = 0;
	si.nSubMeshCount = 0;
	si.nNumRefs = 0;
	si.bSplitLods = false;
	// Analyze geom object.

	bool bMultiSubObj = (si.pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND) != 0;

	si.nMeshSize = 0;
	si.nTextureSize = 0;
	si.nIndices = 0;
	si.nVertices = 0;
	si.nPhysProxySize = 0;
	si.nPhysPrimitives = 0;

	m_stats.nStatObj_TotalCount++;


	IStatObj::SStatistics stats;
	stats.pTextureSizer = &statObjTextureSizer;
	stats.pTextureSizer2 = &localTextureSizer;
	

	si.pStatObj->GetStatistics( stats );

	si.nVertices = stats.nVertices;
	si.nIndices = stats.nIndices;
	for (int i = 0; i < MAX_STATOBJ_LODS_NUM; i++)
		si.nIndicesPerLod[i] = stats.nIndicesPerLod[i];
	si.nMeshSize = stats.nMeshSize;
	si.nMeshSizeLoaded = stats.nMeshSizeLoaded;
	si.nPhysProxySize = stats.nPhysProxySize;
	si.nPhysPrimitives = stats.nPhysPrimitives;
	si.nLods = stats.nLods;
	si.nSubMeshCount = stats.nSubMeshCount;
	si.nNumRefs = stats.nNumRefs;
	si.bSplitLods = stats.bSplitLods;

	si.nTextureSize = localTextureSizer.GetTotalSize();

	m_stats.nStatObj_SummaryMeshSize += si.nMeshSize;
	m_stats.objects.push_back(si); 
}


inline bool CompareAnimations( const SAnimationStatistics &p1,const SAnimationStatistics &p2 )
{
	return p1.count > p2.count;
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectLoadingData()
{
	CLoadingProfilerSystem::FillProfilersList(m_stats.loading);
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectAnimations()
{
	ISystem *pSystem = GetISystem();
	I3DEngine *p3DEngine = pSystem->GetI3DEngine();

	m_stats.animations.clear();
/*
	size_t count = pSystem->GetIAnimationSystem()->GetIAnimEvents()->GetGlobalAnimCount();

	//m_stats.animations.reserve(count);
	for (size_t i = 0; i < count; ++i) {
		SAnimationStatistics stat;
		pSystem->GetIAnimationSystem()->GetIAnimEvents()->GetGlobalAnimStatistics(i, stat);		
		if (stat.count)
			m_stats.animations.push_back(stat);
	}
	std::sort( m_stats.animations.begin(),m_stats.animations.end(),CompareAnimations );
	*/

}

void GetObjectsByType(EERType objectType, std::vector<IRenderNode*>& lstInstances)
{
	I3DEngine *p3DEngine = GetISystem()->GetI3DEngine();

	const uint32 dwCount = p3DEngine->GetObjectsByType(objectType);
	if(dwCount)
	{
		const uint32 numObjects = lstInstances.size();
		lstInstances.resize(numObjects + dwCount);
		p3DEngine->GetObjectsByType(objectType, &lstInstances[numObjects]);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectGeometry()
{
	ISystem *pSystem = GetISystem();
	I3DEngine *p3DEngine = pSystem->GetI3DEngine();

	m_stats.nStatObj_SummaryTextureSize = 0;
	m_stats.nStatObj_SummaryMeshSize = 0;
	m_stats.nStatObj_TotalCount = 0;


	CrySizerImpl statObjTextureSizer;

/*
	SCryEngineStats::StatObjInfo si;
	si.nLods = 0;
	si.nSubMeshCount = 0;
	// Analyze geom object.
	si.nMeshSize = 0;
	si.nTextureSize = 0;
	si.nIndices = 0;
	si.nVertices = 0;
	si.nPhysProxySize = 0;
	si.nPhysPrimitives = 0;
*/
	// iterate through all IStatObj
	{
		int nObjCount = 0;

		p3DEngine->GetLoadedStatObjArray(0,nObjCount);
		if (nObjCount > 0)
		{
			m_stats.objects.reserve(nObjCount);
			IStatObj **pObjects = new IStatObj*[nObjCount];
			p3DEngine->GetLoadedStatObjArray(pObjects,nObjCount);
			
			for (int nCurObj = 0; nCurObj < nObjCount; nCurObj++)
				AddResource_StatObjWithLODs(pObjects[nCurObj],statObjTextureSizer,0);

			delete []pObjects;
		}
	}

	// iterate through all instances
	{
		std::vector<IRenderNode*>	lstInstances;

		GetObjectsByType(eERType_Brush, lstInstances);
		GetObjectsByType(eERType_Vegetation, lstInstances);
		GetObjectsByType(eERType_Light, lstInstances);
		GetObjectsByType(eERType_Decal, lstInstances);

		std::vector<IRenderNode*>::const_iterator itEnd = lstInstances.end();
		for(std::vector<IRenderNode*>::iterator it = lstInstances.begin(); it != itEnd; ++it)
		{
			IRenderNode *pRenderNode = *it;

			const int slotCount = pRenderNode->GetSlotCount();
			for(uint32 dwSlot=0; dwSlot < slotCount; ++dwSlot)			
			{
				if(IStatObj *pEntObject = pRenderNode->GetEntityStatObj(dwSlot))
				{
					m_ResourceCollector.AddInstance(pEntObject->GetFilePath(),pRenderNode);

					if(IMaterial *pMat = pRenderNode->GetMaterial())		// if this rendernode overwrites the IStatObj material
					{
						m_ResourceCollector.OpenDependencies(pEntObject->GetFilePath());
						AddResource_Material(*pMat);		// to report the dependencies of this instance to the IStatObj
						m_ResourceCollector.CloseDependencies();
					}
				}

				if(ICharacterInstance *pCharInst = pRenderNode->GetEntityCharacter(dwSlot))
					m_ResourceCollector.AddInstance(pCharInst->GetFilePath(),pRenderNode);
			}
		}
	}

	m_stats.nStatObj_SummaryTextureSize += statObjTextureSizer.GetTotalSize();
	std::sort( m_stats.objects.begin(),m_stats.objects.end(),CompareStatObjBySizeFunc );
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectCharacters()
{
	ISystem *pSystem = GetISystem();
	I3DEngine *p3DEngine = pSystem->GetI3DEngine();

	m_stats.nChar_SummaryTextureSize = 0;
	m_stats.nChar_SummaryMeshSize = 0;
	m_stats.nChar_NumInstances = 0;

	CrySizerImpl totalCharactersTextureSizer;

	int nObjCount = 0;
	pSystem->GetIAnimationSystem()->GetLoadedModels(0,nObjCount);
	if (nObjCount > 0)
	{
		m_stats.characters.reserve(nObjCount);
		ICharacterModel **pObjects = new ICharacterModel*[nObjCount];
		pSystem->GetIAnimationSystem()->GetLoadedModels(pObjects,nObjCount);
		for (int nCurObj = 0; nCurObj < nObjCount; nCurObj++)
		{
			if (!pObjects[nCurObj])
				continue;

			// Do not consider cga files characters (they are already considered to be static geometries)
			//if (stricmp(PathUtil::GetExt(pObjects[nCurObj]->GetModelFilePath()),"cga") == 0)
			//	continue;

			SCryEngineStats::CharacterInfo si;
			si.pModel = pObjects[nCurObj];
			if (!si.pModel)
				continue;

			// dependencies
//			AddResource(*si.pModel);

			CrySizerImpl textureSizer;

			si.nInstances = si.pModel->GetNumInstances();
			si.nLods = si.pModel->GetNumLods();
			si.nIndices = 0;
			si.nVertices = 0;
			memset(si.nVerticesPerLod,0,sizeof(si.nVerticesPerLod));
			memset(si.nIndicesPerLod,0,sizeof(si.nIndicesPerLod));

			CrySizerImpl meshSizer;

			si.nMeshSize = si.pModel->GetMeshMemoryUsage(&meshSizer);
			si.pModel->GetTextureMemoryUsage( &textureSizer );
			si.pModel->GetTextureMemoryUsage( &totalCharactersTextureSizer );
			si.nPhysProxySize = 0;
			bool bLod0_Found = false;
			for (int nlod = 0; nlod < MAX_LODS; nlod++)
			{
				IRenderMesh *pRenderMesh = si.pModel->GetRenderMesh(nlod);
				if (pRenderMesh)
				{
					if (!bLod0_Found)
					{
						bLod0_Found = true;
						si.nVertices = pRenderMesh->GetVerticesCount();
						si.nIndices = pRenderMesh->GetIndicesCount();
					}
					si.nVerticesPerLod[nlod] = pRenderMesh->GetVerticesCount();
					si.nIndicesPerLod[nlod] = pRenderMesh->GetIndicesCount();
				}
				phys_geometry *pgeom;
				if(nlod<2) for(int i=si.pModel->GetNumJoints()-1; i>=0; i--)
					if (pgeom = si.pModel->GetJointPhysGeom((uint32)i,nlod))
					{
						CrySizerImpl physMeshSizer;
						pgeom->pGeom->GetMemoryStatistics(&physMeshSizer);
						si.nPhysProxySize += physMeshSizer.GetTotalSize(); 
					}
			}
		
			si.nTextureSize = textureSizer.GetTotalSize();

			m_stats.nChar_SummaryMeshSize += si.nMeshSize;
			m_stats.characters.push_back(si);

			m_stats.nChar_NumInstances += si.nInstances;
		}
		delete []pObjects;
	}
	m_stats.nChar_SummaryTextureSize = totalCharactersTextureSizer.GetTotalSize();
	std::sort( m_stats.characters.begin(),m_stats.characters.end(),CompareCharactersBySizeFunc );
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectEntityDependencies()
{
	ISystem *pSystem = GetISystem();

	IEntitySystem *pEntitySystem = pSystem->GetIEntitySystem();

	IEntityItPtr it = pEntitySystem->GetEntityIterator();
	while ( !it->IsEnd() )
	{
		IEntity *pEntity = it->Next();

		IMaterial *pMat=pEntity->GetMaterial();

		if(pMat)
			AddResource_Material(*pMat);

		uint32 dwSlotCount = pEntity->GetSlotCount();

		for(uint32 dwI=0;dwI<dwSlotCount;++dwI)
		{
			SEntitySlotInfo slotInfo;

			if(pEntity->GetSlotInfo(dwI,slotInfo))
			{
				if(slotInfo.pMaterial)
					AddResource_Material(*slotInfo.pMaterial);

				if(slotInfo.pCharacter)
					AddResource_CharInstance(*slotInfo.pCharacter);
			}
		}
	}	
}


//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectMaterialDependencies()
{
	ISystem *pSystem = GetISystem();
	I3DEngine *p3DEngine = pSystem->GetI3DEngine();

	IMaterialManager *pManager = p3DEngine->GetMaterialManager();

	uint32 nObjCount=0;
	pManager->GetLoadedMaterials(0,nObjCount);

	if(nObjCount>0)
	{
		std::vector<IMaterial *> Materials;

		Materials.resize(nObjCount);

		pManager->GetLoadedMaterials(&Materials[0],nObjCount);
	
		std::vector<IMaterial *>::const_iterator it, end=Materials.end();

		for(it=Materials.begin();it!=end;++it)
		{
			IMaterial *pMat = *it;

			AddResource_Material(*pMat);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectTextures()
{
	ISystem *pSystem = GetISystem();
	IRenderer *pRenderer = pSystem->GetIRenderer();

	m_stats.nSummary_TextureSize = 0;
	m_stats.nSummary_UserTextureSize = 0;
	m_stats.nSummary_EngineTextureSize = 0;
	m_stats.nSummary_TexturesStreamingThroughput = 0;
	m_stats.nSummary_TexturesPoolSize = *(int*)pRenderer->EF_Query(EFQ_TexturesPoolSize,0);

	m_stats.textues.clear();
	int nTexCount = *(int*)pRenderer->EF_Query(EFQ_GetAllTextures,0);
	if (nTexCount > 0)
	{
		m_stats.textues.reserve(nTexCount);
		ITexture **pTextures = new ITexture*[nTexCount];
		pRenderer->EF_Query( EFQ_GetAllTextures,(INT_PTR)pTextures );
		for (int i = 0; i < nTexCount; i++)
		{
			ITexture *pTexture = pTextures[i];
			int nTexSize = pTexture->GetDataSize();
			if (nTexSize > 0)
			{
				m_stats.textues.push_back( pTexture );

//#ifdef XENON
//				if (pTexture->GetFlags() & FT_DONT_STREAM)
//#endif
				m_stats.nSummary_TextureSize += nTexSize;

				if (pTexture->GetFlags() & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET))
				{
					m_stats.nSummary_EngineTextureSize += nTexSize;
				}
				else
				{
#ifdef XENON
					if (!pTexture->IsStreamable())
#endif
						m_stats.nSummary_UserTextureSize += nTexSize;

				}
			}
		}
		delete []pTextures;

		std::sort( m_stats.textues.begin(),m_stats.textues.end(),CompareTexturesBySizeFunc );
	}

	STextureStreamingStats stats;
	m_stats.nSummary_TexturesStreamingThroughput = 0;
	if( pRenderer->EF_Query(EFQ_GetTexStreamingInfo, (INT_PTR)&stats ) )
		m_stats.nSummary_TexturesStreamingThroughput = (float)stats.nThroughput;
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectMaterials()
{
  ISystem *pSystem = GetISystem();
  I3DEngine *p3DEngine = pSystem->GetI3DEngine();

  IMaterialManager *pManager = p3DEngine->GetMaterialManager();

  uint32 nObjCount=0;
  pManager->GetLoadedMaterials(0,nObjCount);

  m_stats.materials.resize(nObjCount);

  if(nObjCount>0)
  {
    pManager->GetLoadedMaterials(&m_stats.materials[0],nObjCount);

    std::sort( m_stats.materials.begin(),m_stats.materials.end(),CompareMaterialsByName );
  }
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectRenderMeshes()
{
	ISystem *pSystem = GetISystem();
	IRenderer *pRenderer = pSystem->GetIRenderer();

	m_stats.meshes.clear();
	m_stats.meshes.reserve(100);

	int nVertices = 0;
	int nIndices = 0;

	int nCount = *(int*)pRenderer->EF_Query(EFQ_GetAllMeshes,0);
	if (nCount > 0)
	{
		IRenderMesh **pMeshes = new IRenderMesh*[nCount];
		pRenderer->EF_Query( EFQ_GetAllMeshes,(INT_PTR)pMeshes );
		
		// Sort meshes by name.
		std::sort( pMeshes,pMeshes+nCount,CompareRenderMeshByTypeName );

		CrySizerImpl* pTextureSizer = new CrySizerImpl();

		int nInstances = 0;
		int nMeshSizeSys = 0;
    int nMeshSizeDev = 0;
		const char *sMeshName = 0;
		const char *sLastMeshName = "";
		for (int i = 0; i < nCount; i++)
		{
			IRenderMesh *pMesh = pMeshes[i];
			sMeshName = pMesh->GetTypeName();

			if ((strcmp(sMeshName,sLastMeshName) != 0 && i != 0))
			{
				SCryEngineStats::MeshInfo mi;
				mi.nCount = nInstances;
				mi.nVerticesSum = nVertices;
				mi.nIndicesSum = nIndices;
				mi.nMeshSizeDev = nMeshSizeDev;
        mi.nMeshSizeSys = nMeshSizeSys;
				mi.nTextureSize = (int)pTextureSizer->GetTotalSize();
				mi.name = sLastMeshName;
				m_stats.meshes.push_back(mi);

				delete pTextureSizer;
				pTextureSizer = new CrySizerImpl();
				nInstances = 0;
				nMeshSizeSys = 0;
        nMeshSizeDev = 0;
				nVertices = 0;
				nIndices = 0;
			}
			sLastMeshName = sMeshName;
			nMeshSizeSys += pMesh->GetMemoryUsage(0,IRenderMesh::MEM_USAGE_ONLY_SYSTEM); // Collect System+Video memory usage.
      nMeshSizeDev += pMesh->GetMemoryUsage(0,IRenderMesh::MEM_USAGE_ONLY_VIDEO); // Collect System+Video memory usage.
			pMesh->GetTextureMemoryUsage(pMesh->GetMaterial(), pTextureSizer);

			nVertices += pMesh->GetVerticesCount();
			nIndices += pMesh->GetIndicesCount();

			nInstances++;
		}
		if (nCount > 0 && sMeshName)
		{
			SCryEngineStats::MeshInfo mi;
			mi.nCount = nInstances;
			mi.nVerticesSum = nVertices;
			mi.nIndicesSum = nIndices;
			mi.nMeshSizeSys = nMeshSizeSys;
      mi.nMeshSizeDev = nMeshSizeDev;
			mi.nTextureSize = (int)pTextureSizer->GetTotalSize();
			mi.name = sMeshName;
			m_stats.meshes.push_back(mi);
		}

		delete pTextureSizer;

		delete []pMeshes;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectVoxels()
{
	//int nCount = 10000;
	//IRenderNode *pRenderNodes = NULL;
	
	//virtual void GetVoxelRenderNodes(struct IRenderNode**pRenderNodes, int & nCount) = 0;
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectWavebanks()
{
	ISystem *pSystem = GetISystem();
	ISoundSystem *pSoundSystem = gEnv->pSoundSystem;

	if(!pSoundSystem->GetIAudioDevice())
		return;															// rare condition (one device exists but cannot be initialized)

	m_stats.wavebanks.clear();

	int nWavebankCount = pSoundSystem->GetIAudioDevice() ? pSoundSystem->GetIAudioDevice()->GetWavebankCount() : 0;
	if (nWavebankCount > 0)
	{
		m_stats.wavebanks.reserve(nWavebankCount);

		for (int i=0; i<nWavebankCount; ++i)
		{
			IWavebank *pWavebank = pSoundSystem->GetIAudioDevice()->GetWavebank(i);
			m_stats.wavebanks.push_back(pWavebank);
		}

		std::sort( m_stats.wavebanks.begin(),m_stats.wavebanks.end(),CompareWavebanksBySizeFunc );
	}
	
}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectSoundInfos()
{
	ISystem *pSystem = GetISystem();
	ISoundSystem *pSoundSystem = gEnv->pSoundSystem;

	m_stats.soundinfos.clear();

	int nSoundCount = pSoundSystem->GetInterfaceExtended()->GetSoundInfoCount();
	if (nSoundCount > 0)
	{
		m_stats.soundinfos.reserve(nSoundCount);

		for (int i=0; i<nSoundCount; ++i)
		{
			ISoundProfileInfo *pSoundInfo = pSoundSystem->GetInterfaceExtended()->GetSoundInfo(i);
			m_stats.soundinfos.push_back(pSoundInfo);
		}

		std::sort( m_stats.soundinfos.begin(), m_stats.soundinfos.end(),CompareSoundInfosByTimesPlayedFunc );
	}

}

//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectProfileStatistics()
{
	ISystem *pSystem = GetISystem();
	IFrameProfileSystem* pProfiler = pSystem->GetIProfileSystem();
	
	m_stats.profilers.clear();

	uint32 num = pProfiler->GetProfilerCount();//min(20, pProfiler->GetProfilerCount());

	int need = 0;

	for (uint32 i = 0; i < num; ++i)
	{
		CFrameProfiler * pFrameInfo = pProfiler->GetProfiler(i);
		if (pFrameInfo && pFrameInfo->m_countHistory.GetAverage() > 0 && pFrameInfo->m_totalTimeHistory.GetAverage() > 0.0f)
			++need;
	}
	

	m_stats.profilers.resize(need);
	for (uint32 j = 0, i = 0; j < num; ++j)
	{
		CFrameProfiler * pFrameInfo = pProfiler->GetProfiler(j);	

		if (pFrameInfo && pFrameInfo->m_countHistory.GetAverage() > 0 && pFrameInfo->m_totalTimeHistory.GetAverage() > 0.0f)
		{

			m_stats.profilers[i].m_count =  pFrameInfo->m_countHistory.GetAverage();//pFrameInfo->m_count; 
//			m_stats.profilers[i].m_displayedCurrentValue = pFrameInfo->m_displayedCurrentValue;
			m_stats.profilers[i].m_displayedValue = pFrameInfo->m_selfTimeHistory.GetAverage();
			m_stats.profilers[i].m_name = pFrameInfo->m_name;
			m_stats.profilers[i].m_module = ((CFrameProfileSystem*)pProfiler)->GetModuleName(pFrameInfo);
			m_stats.profilers[i].m_selfTime = pFrameInfo->m_selfTime;
			m_stats.profilers[i].m_sumSelfTime = pFrameInfo->m_sumSelfTime;
			m_stats.profilers[i].m_sumTotalTime = pFrameInfo->m_sumTotalTime;
			m_stats.profilers[i].m_totalTime = pFrameInfo->m_totalTimeHistory.GetAverage();
			m_stats.profilers[i].m_variance = pFrameInfo->m_variance;
			m_stats.profilers[i].m_min = (float)pFrameInfo->m_selfTimeHistory.GetMin();
			m_stats.profilers[i].m_max = (float)pFrameInfo->m_selfTimeHistory.GetMax();
			m_stats.profilers[i].m_mincount = pFrameInfo->m_countHistory.GetMin();
			m_stats.profilers[i].m_maxcount = pFrameInfo->m_countHistory.GetMax();
			i++;
		}
	}

	std::sort( m_stats.profilers.begin(),m_stats.profilers.end(),CompareFrameProfilersValueStats );

	// fill peaks
	num= pProfiler->GetPeaksCount();

	m_stats.peaks.resize(num);
	for (uint32 i = 0; i < num; ++i)
	{

		const SPeakRecord * pPeak = pProfiler->GetPeak(i);
		CFrameProfiler * pFrameInfo = pPeak->pProfiler;	

		m_stats.peaks[i].peakValue = pPeak->peakValue;
		m_stats.peaks[i].avarageValue = pPeak->avarageValue;
		m_stats.peaks[i].variance = pPeak->variance;
		m_stats.peaks[i].pageFaults = pPeak->pageFaults; // Number of page faults at this frame.
		m_stats.peaks[i].count = pPeak->count;  // Number of times called for peak.
		m_stats.peaks[i].when = pPeak->when; // when it added.

		m_stats.peaks[i].profiler.m_count =  pFrameInfo->m_countHistory.GetAverage();//pFrameInfo->m_count; 
//		m_stats.peaks[i].profiler.m_displayedCurrentValue = pFrameInfo->m_displayedCurrentValue;
		m_stats.peaks[i].profiler.m_displayedValue = pFrameInfo->m_selfTimeHistory.GetAverage();
		m_stats.peaks[i].profiler.m_name = pFrameInfo->m_name;
		m_stats.peaks[i].profiler.m_module = ((CFrameProfileSystem*)pProfiler)->GetModuleName(pFrameInfo);
		m_stats.peaks[i].profiler.m_selfTime = pFrameInfo->m_selfTime;
		m_stats.peaks[i].profiler.m_sumSelfTime = pFrameInfo->m_sumSelfTime;
		m_stats.peaks[i].profiler.m_sumTotalTime = pFrameInfo->m_sumTotalTime;
		m_stats.peaks[i].profiler.m_totalTime = pFrameInfo->m_totalTimeHistory.GetAverage();
		m_stats.peaks[i].profiler.m_variance = pFrameInfo->m_variance;
		m_stats.peaks[i].profiler.m_min = (float)pFrameInfo->m_selfTimeHistory.GetMin();
		m_stats.peaks[i].profiler.m_max = (float)pFrameInfo->m_selfTimeHistory.GetMax();
		m_stats.peaks[i].profiler.m_mincount = pFrameInfo->m_countHistory.GetMin();
		m_stats.peaks[i].profiler.m_maxcount = pFrameInfo->m_countHistory.GetMax();
	}

	int modules = ((CFrameProfileSystem*)pProfiler)->GetModuleCount();
	m_stats.moduleprofilers.resize(modules);

	for (int i=0;i<modules;i++)
	{
		float ratio = ((CFrameProfileSystem*)pProfiler)->GetOverBudgetRatio(i);
		m_stats.moduleprofilers[i].name =((CFrameProfileSystem*)pProfiler)->GetModuleName(i);
		m_stats.moduleprofilers[i].overBugetRatio =ratio;
	}

}

//////////////////////////////////////////////////////////////////////////
#if defined(WIN32)

/*static*/ bool QueryModuleMemoryInfo( SCryEngineStatsModuleInfo &moduleInfo,int index )
{
	HMODULE hModule = GetModuleHandle( moduleInfo.name );
	if (!hModule)
		return false;

	typedef void (*PFN_MODULEMEMORY)( CryModuleMemoryInfo* );
	PFN_MODULEMEMORY fpCryModuleGetAllocatedMemory = (PFN_MODULEMEMORY)::GetProcAddress( hModule,"CryModuleGetMemoryInfo" );
	if (!fpCryModuleGetAllocatedMemory)
		return false;

	PEHeader_DLL pe_header;
	PEHeader_DLL *header = &pe_header;

	const IMAGE_DOS_HEADER *dos_head = (IMAGE_DOS_HEADER*)hModule;
	if (dos_head->e_magic != IMAGE_DOS_SIGNATURE)
	{
		// Wrong pointer, not to PE header.
		return false;
	}
	header = (PEHeader_DLL*)(const void *)((char *)dos_head + dos_head->e_lfanew);
	moduleInfo.moduleStaticSize = header->opt_head.SizeOfInitializedData + header->opt_head.SizeOfUninitializedData + header->opt_head.SizeOfCode + header->opt_head.SizeOfHeaders;
	moduleInfo.SizeOfCode = header->opt_head.SizeOfCode;
	moduleInfo.SizeOfInitializedData = header->opt_head.SizeOfInitializedData;
	moduleInfo.SizeOfUninitializedData = header->opt_head.SizeOfUninitializedData;

	fpCryModuleGetAllocatedMemory( &moduleInfo.memInfo );

	moduleInfo.usedInModule = (int)(moduleInfo.memInfo.allocated - moduleInfo.memInfo.freed);
	return true;
}

#elif defined(XENON)

/*static */bool QueryModuleMemoryInfo( SCryEngineStatsModuleInfo &moduleInfo,int index )
{
	if (strcmp(moduleInfo.name,"XBoxOS") == 0)
	{
		moduleInfo.usedInModule = 36*1024*1024; // On Xbox 360 OS uses 36 MB
		return true;
	}

	HMODULE hModule = GetModuleHandle( moduleInfo.name );
	if (!hModule)
		return false;

	typedef void (*PFN_MODULEMEMORY)( CryModuleMemoryInfo* );
	PFN_MODULEMEMORY fpCryModuleGetAllocatedMemory = (PFN_MODULEMEMORY)::GetProcAddress( hModule, (LPCSTR)8 );
	if (!fpCryModuleGetAllocatedMemory)
		return false;

	HRESULT error;
	PDM_WALK_MODULES pWalkMod = NULL;
	DMN_MODLOAD modLoad;
	while( XBDM_NOERR == (error = DmWalkLoadedModules(&pWalkMod, &modLoad)) ) {
		if (_stricmp(moduleInfo.name.c_str(), modLoad.Name) == 0) {
			// This is it
			moduleInfo.SizeOfCode = modLoad.Size;
			moduleInfo.SizeOfInitializedData = modLoad.PDataSize;
			moduleInfo.SizeOfUninitializedData = 0;
			moduleInfo.moduleStaticSize = moduleInfo.SizeOfCode + moduleInfo.SizeOfInitializedData;
			break;

		}
	}
	if (error != XBDM_ENDOFLIST)
	{
		// Handle error.
	}
	DmCloseLoadedModules(pWalkMod);

	fpCryModuleGetAllocatedMemory( &moduleInfo.memInfo );
	moduleInfo.usedInModule = (int)(moduleInfo.memInfo.allocated - moduleInfo.memInfo.freed);
	return true;
}

#elif defined(PS3)

/*static */bool QueryModuleMemoryInfo( SCryEngineStatsModuleInfo &moduleInfo,int index )
{
	if (strcmp(moduleInfo.name,"PS3OS") == 0)
	{
		moduleInfo.usedInModule = 46*1024*104;  // On PS3 OS uses 46 MB
		return true;
	}

	CryModuleGetMemoryInfo( &moduleInfo.memInfo, (ECryModule)index );
#if defined(SUPP_MTRACE)
  MTrace::SystemStats sys_stats; 
  MTrace::SystemStatistics(sys_stats); 
  moduleInfo.memInfo.allocated				= sys_stats.modules[(ECryModule)index].allocated; 
  moduleInfo.memInfo.freed						= sys_stats.modules[(ECryModule)index].freed;
  moduleInfo.memInfo.num_allocations	= sys_stats.modules[(ECryModule)index].num_allocations;
	moduleInfo.memInfo.requested				= 
  moduleInfo.usedInModule							= sys_stats.modules[(ECryModule)index].current_memory;
#endif
	return true;
}
#else //Another platform
/*static */bool QueryModuleMemoryInfo( SCryEngineStatsModuleInfo &moduleInfo,int index )
{
	//CryModuleGetMemoryInfo( &moduleInfo.memInfo, (ECryModule)index );
	CryModuleGetMemoryInfo(&moduleInfo.memInfo);
	moduleInfo.usedInModule = (int)(moduleInfo.memInfo.allocated - moduleInfo.memInfo.freed);
	return true;
}

#endif //defined(PS3)

const std::vector<const char*>& GetModuleNames()
{
	static std::vector<const char*> moduleNames;

#if !defined(PS3) || (defined(PS3) && defined(_LIB))

	if(moduleNames.empty())
	{

#	ifdef MODULE_EXTENSION
#		error MODULE_EXTENSION already defined!
#	endif
#	ifdef PS3
#		define MODULE_EXTENSION
#	else
#		define MODULE_EXTENSION ".dll"
# endif
# ifdef LINUX
#		define MODULE_EXTENSION ".so"
# endif

		moduleNames.push_back("Cry3DEngine" MODULE_EXTENSION);
		moduleNames.push_back("CryAction" MODULE_EXTENSION);
		moduleNames.push_back("CryAISystem" MODULE_EXTENSION);
		moduleNames.push_back("CryAnimation" MODULE_EXTENSION);
		moduleNames.push_back("CryEntitySystem" MODULE_EXTENSION);
		moduleNames.push_back("CryFont" MODULE_EXTENSION);
		moduleNames.push_back("CryInput" MODULE_EXTENSION);
		moduleNames.push_back("CryMovie" MODULE_EXTENSION);
		moduleNames.push_back("CryNetwork" MODULE_EXTENSION);
		moduleNames.push_back("CryPhysics" MODULE_EXTENSION);
		moduleNames.push_back("CryScriptSystem" MODULE_EXTENSION);
		moduleNames.push_back("CrySoundSystem" MODULE_EXTENSION);
		moduleNames.push_back("CrySystem" MODULE_EXTENSION);
		moduleNames.push_back("Game" MODULE_EXTENSION);
    // K01
		moduleNames.push_back("CryOnline" MODULE_EXTENSION);

#undef MODULE_EXTENSION

#	ifdef PS3
		moduleNames.push_back("Render");
		moduleNames.push_back("PS3Launcher");
//		moduleNames.push_back("PS3OS");
#	elif defined(LINUX)
		moduleNames.push_back("CryRenderNULL.so");
#	else
		moduleNames.push_back("Editor.exe");
		moduleNames.push_back("CryRenderD3D9.dll");
		moduleNames.push_back("CryRenderD3D10.dll");
		moduleNames.push_back("CryRenderNULL.dll");
		//TODO: launcher? xenon launcher?

		// The OS has to be last!
#		ifdef XENON
		moduleNames.push_back("XBoxOS");
#		endif
#	endif
	}
#else
	CryFatalError("Can't get module names on PS3");
#endif

	return moduleNames;
}


//////////////////////////////////////////////////////////////////////////
void CEngineStats::CollectMemInfo()
{
	m_stats.memInfo.totalUsedInModules = 0;
	m_stats.memInfo.totalCodeAndStatic = 0;
	m_stats.memInfo.countedMemoryModules = 0;
	m_stats.memInfo.totalAllocatedInModules = 0;
	m_stats.memInfo.totalNumAllocsInModules = 0;

#if !defined(PS3) || (defined(PS3) && defined(_LIB))
#ifdef PS3
	m_stats.memInfo.totalCodeAndStatic = gPS3Env->staticMemUsedKB << 10;
#endif
	const std::vector<const char*>& szModules = GetModuleNames();
	const int numModules = szModules.size();
	
	//////////////////////////////////////////////////////////////////////////
	// Hardcoded value for the OS memory allocation.
	//////////////////////////////////////////////////////////////////////////
	for (int i = 0; i < numModules; i++)
{
		const char *szModule = szModules[i];

 		SCryEngineStatsModuleInfo moduleInfo;
		ZeroStruct( moduleInfo.memInfo );
		moduleInfo.moduleStaticSize = moduleInfo.SizeOfCode = moduleInfo.SizeOfInitializedData = moduleInfo.SizeOfUninitializedData = moduleInfo.usedInModule = 0;
		moduleInfo.name = szModule;

		if (!QueryModuleMemoryInfo( moduleInfo,i ))
			continue;

		m_stats.memInfo.totalNumAllocsInModules += moduleInfo.memInfo.num_allocations;
		m_stats.memInfo.totalAllocatedInModules += moduleInfo.memInfo.allocated;
		m_stats.memInfo.totalUsedInModules += moduleInfo.usedInModule;
		m_stats.memInfo.countedMemoryModules++;
		m_stats.memInfo.totalCodeAndStatic += moduleInfo.moduleStaticSize;

		m_stats.memInfo.modules.push_back(moduleInfo);
	}
#endif
	m_stats.memInfo.m_pSizer = new CrySizerImpl();
	((CSystem*)GetISystem())->CollectMemStats( m_stats.memInfo.m_pSizer,CSystem::nMSP_ForDump );
	m_stats.memInfo.m_pStats = new CrySizerStats(m_stats.memInfo.m_pSizer);
}

// Exports engine stats to the Excel.
class CStatsToExcelExporter
{
public:
	enum CellFlags
	{
		CELL_BOLD = 0x0001,
		CELL_CENTERED = 0x0002,
	};
	void ExportToFile( SCryEngineStats &stats,const char *filename );
	void ExportDependenciesToFile( CResourceCollector &stats,const char *filename );
	void Export( XmlNodeRef Workbook,SCryEngineStats &stats );

private:
	void ExportSummary( SCryEngineStats &stats );
	void ExportStatObjects( SCryEngineStats &stats );
	void ExportCharacters( SCryEngineStats &stats );
	void ExportRenderMeshes( SCryEngineStats &stats );
	void ExportTextures( SCryEngineStats &stats );
	void ExportTextureAllocations();
  void ExportMaterials( SCryEngineStats &stats );
	void ExportWavebanks( SCryEngineStats &stats );
	void ExportSoundInfos( SCryEngineStats &stats );
	void ExportMemStats(SCryEngineStats &stats );
	void ExportMemInfo( SCryEngineStats &stats );
	void ExportTimeDemoInfo();
	void ExportDependencies( CResourceCollector &stats );
	void ExportProfilerStatistics( SCryEngineStats &stats );
	void ExportAnimationStatistics( SCryEngineStats &stats );
	void ExportAllLoadingStatistics( SCryEngineStats &stats ); 
	void ExportLoadingStatistics( SCryEngineStats &stats); 
	void ExportFPSBuckets();

	void InitExcelWorkbook( XmlNodeRef Workbook );

	XmlNodeRef NewWorksheet( const char *name );
	void FreezeFirstRow();
	void AutoFilter( int nRow,int nNumColumns );
	void AddCell( float number );
	void AddCell( int number );
	void AddCell( uint32 number );
	void AddCell( uint64 number ) { AddCell( (uint32)number ); };
	void AddCell( int64 number ) { AddCell( (int)number ); };
	void AddCell( const char *str,int flags=0 );
	void AddCellAtIndex( int nIndex,const char *str,int flags=0 );
	void SetCellFlags( XmlNodeRef cell,int flags );
	void AddRow();
	void AddCell_SumOfRows( int nRows );
	string GetXmlHeader();

private:
	XmlNodeRef m_Workbook;
	XmlNodeRef m_CurrTable;
	XmlNodeRef m_CurrWorksheet;
	XmlNodeRef m_CurrRow;
	XmlNodeRef m_CurrCell;
};

//////////////////////////////////////////////////////////////////////////
string CStatsToExcelExporter::GetXmlHeader()
{
	return "<?xml version=\"1.0\"?>\n<?mso-application progid=\"Excel.Sheet\"?>\n";
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::InitExcelWorkbook( XmlNodeRef Workbook )
{
	m_Workbook = Workbook;
	m_Workbook->setTag( "Workbook" );
	m_Workbook->setAttr( "xmlns","urn:schemas-microsoft-com:office:spreadsheet" );
	XmlNodeRef ExcelWorkbook = Workbook->newChild( "ExcelWorkbook" );
	ExcelWorkbook->setAttr( "xmlns","urn:schemas-microsoft-com:office:excel" );

	XmlNodeRef Styles = m_Workbook->newChild("Styles");
	{
		// Style s25
		// Bold header, With Background Color.
		XmlNodeRef Style = Styles->newChild("Style");
		Style->setAttr("ss:ID","s25");
		XmlNodeRef StyleFont = Style->newChild("Font");
		StyleFont->setAttr("x:CharSet","204");
		StyleFont->setAttr("x:Family","Swiss");
		StyleFont->setAttr("ss:Bold","1");
		XmlNodeRef StyleInterior = Style->newChild("Interior");
		StyleInterior->setAttr("ss:Color","#00FF00");
		StyleInterior->setAttr("ss:Pattern","Solid");
		XmlNodeRef NumberFormat = Style->newChild( "NumberFormat" );
		NumberFormat->setAttr( "ss:Format","#,##0" );
	}
	{
		// Style s26
		// Bold/Centered header.
		XmlNodeRef Style = Styles->newChild("Style");
		Style->setAttr("ss:ID","s26");
		XmlNodeRef StyleFont = Style->newChild("Font");
		StyleFont->setAttr("x:CharSet","204");
		StyleFont->setAttr("x:Family","Swiss");
		StyleFont->setAttr("ss:Bold","1");
		XmlNodeRef StyleInterior = Style->newChild("Interior");
		StyleInterior->setAttr("ss:Color","#FFFF99");
		StyleInterior->setAttr("ss:Pattern","Solid");
		XmlNodeRef Alignment = Style->newChild( "Alignment" );
		Alignment->setAttr( "ss:Horizontal","Center" );
		Alignment->setAttr( "ss:Vertical","Bottom" );
	}
	{
		// Style s20
		// Centered
		XmlNodeRef Style = Styles->newChild("Style");
		Style->setAttr("ss:ID","s20");
		XmlNodeRef Alignment = Style->newChild( "Alignment" );
		Alignment->setAttr( "ss:Horizontal","Center" );
		Alignment->setAttr( "ss:Vertical","Bottom" );
	}
	{
		// Style s21
		// Bold
		XmlNodeRef Style = Styles->newChild("Style");
		Style->setAttr("ss:ID","s21");
		XmlNodeRef StyleFont = Style->newChild("Font");
		StyleFont->setAttr("x:CharSet","204");
		StyleFont->setAttr("x:Family","Swiss");
		StyleFont->setAttr("ss:Bold","1");
	}
	{
		// Style s22
		// Centered, Integer Number format
		XmlNodeRef Style = Styles->newChild("Style");
		Style->setAttr("ss:ID","s22");
		XmlNodeRef Alignment = Style->newChild( "Alignment" );
		Alignment->setAttr( "ss:Horizontal","Center" );
		Alignment->setAttr( "ss:Vertical","Bottom" );
		XmlNodeRef NumberFormat = Style->newChild( "NumberFormat" );
		NumberFormat->setAttr( "ss:Format","#,##0" );
	}
	{
		// Style s23
		// Centered, Float Number format
		XmlNodeRef Style = Styles->newChild("Style");
		Style->setAttr("ss:ID","s23");
		XmlNodeRef Alignment = Style->newChild( "Alignment" );
		Alignment->setAttr( "ss:Horizontal","Center" );
		Alignment->setAttr( "ss:Vertical","Bottom" );
		//XmlNodeRef NumberFormat = Style->newChild( "NumberFormat" );
		//NumberFormat->setAttr( "ss:Format","#,##0" );
	}

/*
		<Style ss:ID="s25">
		<Font x:CharSet="204" x:Family="Swiss" ss:Bold="1"/>
		<Interior ss:Color="#FFFF99" ss:Pattern="Solid"/>
		</Style>
		*/
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CStatsToExcelExporter::NewWorksheet( const char *name )
{
	m_CurrWorksheet = m_Workbook->newChild( "Worksheet" );
	m_CurrWorksheet->setAttr( "ss:Name",name );
	m_CurrTable = m_CurrWorksheet->newChild( "Table" );
	return m_CurrWorksheet;
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::FreezeFirstRow()
{
	XmlNodeRef options = m_CurrWorksheet->newChild( "WorksheetOptions" );
	options->setAttr( "xmlns","urn:schemas-microsoft-com:office:excel" );
	options->newChild( "FreezePanes" );
	options->newChild( "FrozenNoSplit" );
	options->newChild( "SplitHorizontal" )->setContent( "1" );
	options->newChild( "TopRowBottomPane" )->setContent( "1" );
	options->newChild( "ActivePane" )->setContent( "2" );
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::AutoFilter( int nRow,int nNumColumns )
{
	XmlNodeRef options = m_CurrWorksheet->newChild( "AutoFilter" );
	options->setAttr( "xmlns","urn:schemas-microsoft-com:office:excel" );
	string range;
	range.Format( "R%dC1:R%dC%d",nRow,nRow,nNumColumns );
	options->setAttr( "x:Range",range ); // x:Range="R1C1:R1C8"
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::AddRow()
{
	m_CurrRow = m_CurrTable->newChild( "Row" );
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::AddCell_SumOfRows( int nRows )
{
	XmlNodeRef cell = m_CurrRow->newChild("Cell");
	XmlNodeRef data = cell->newChild( "Data" );
	data->setAttr( "ss:Type","Number" );
	data->setContent( "0" );
	m_CurrCell = cell;

	if (nRows > 0)
	{
		char buf[128];
		sprintf_s( buf,"=SUM(R[-%d]C:R[-1]C)",nRows );
		m_CurrCell->setAttr( "ss:Formula",buf );
	}
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::AddCell( float number )
{
	XmlNodeRef cell = m_CurrRow->newChild("Cell");
	cell->setAttr("ss:StyleID","s23"); // Centered
	XmlNodeRef data = cell->newChild( "Data" );
	data->setAttr( "ss:Type","Number" );
	char str[128];
	sprintf_s( str,"%.3f",number );
	data->setContent( str );
	m_CurrCell = cell;
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::AddCell( int number )
{
	XmlNodeRef cell = m_CurrRow->newChild("Cell");
	cell->setAttr("ss:StyleID","s22"); // Centered
	XmlNodeRef data = cell->newChild( "Data" );
	data->setAttr( "ss:Type","Number" );
	char str[128];
	sprintf_s( str,"%d",number );
	data->setContent( str );
	m_CurrCell = cell;
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::AddCell( uint32 number )
{
	XmlNodeRef cell = m_CurrRow->newChild("Cell");
	cell->setAttr("ss:StyleID","s22"); // Centered
	XmlNodeRef data = cell->newChild( "Data" );
	data->setAttr( "ss:Type","Number" );
	char str[128];
	sprintf_s( str,"%u",number );
	data->setContent( str );
	m_CurrCell = cell;
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::AddCell( const char *str,int nFlags )
{
	XmlNodeRef cell = m_CurrRow->newChild("Cell");
	XmlNodeRef data = cell->newChild( "Data" );
	data->setAttr( "ss:Type","String" );
	data->setContent( str );
	SetCellFlags( cell,nFlags );
	m_CurrCell = cell;
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::AddCellAtIndex( int nIndex,const char *str,int nFlags )
{
	XmlNodeRef cell = m_CurrRow->newChild("Cell");
	cell->setAttr( "ss:Index",nIndex );
	XmlNodeRef data = cell->newChild( "Data" );
	data->setAttr( "ss:Type","String" );
	data->setContent( str );
	SetCellFlags( cell,nFlags );
	m_CurrCell = cell;
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::SetCellFlags( XmlNodeRef cell,int flags )
{
	if (flags & CELL_BOLD)
	{
		if (flags & CELL_CENTERED)
			cell->setAttr("ss:StyleID","s26");
		else
			cell->setAttr("ss:StyleID","s21");
	}
	else
	{
		if (flags & CELL_CENTERED)
			cell->setAttr("ss:StyleID","s20");
	}
}

static FILE* HandleFileExport(const char *filename)
{
#if defined(PS3)
	#undef fopen
	FILE *file = NULL;
	CellFsErrno Err	=	cellFsMkdir(SYS_APP_HOME "/" g_szTestResults, CELL_FS_DEFAULT_CREATE_MODE_1);
	if(Err != CELL_FS_SUCCEEDED && Err != CELL_FS_EEXIST)
	{
		printf("SaveLevelStats: cellFsMkdir for \"%s\" failed\n", SYS_APP_HOME "/" g_szTestResults);
	}
	char fileBuf[1024];
	sprintf(fileBuf, SYS_APP_HOME"/%s/%s",g_szTestResults,filename);
	file = fopen(fileBuf, "wt");
#else
#ifdef XENON
#ifdef ENABLE_PROFILERS
	HRESULT hr = DmMapDevkitDrive();
	int err = _mkdir(g_szTestResults);
	assert( err == 0 );
	FILE *file = fopen( string(g_szTestResults) + "\\" + filename,"wb" );
#else
	gEnv->pCryPak->MakeDir( g_szTestResults );
	FILE *file = fopen( string(g_szTestResults) + "\\" + filename,"wb" );
#endif
#else
	string temp = gEnv->pSystem->GetRootFolder();
	temp += g_szTestResults;
	gEnv->pCryPak->MakeDir( temp );
	FILE *file = fopen( temp + "/" + filename,"wb" );
#endif
	#endif
	return file;
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportToFile( SCryEngineStats &stats,const char *filename )
{
	XmlNodeRef Workbook = GetISystem()->CreateXmlNode("Workbook");
	Export( Workbook,stats );
	string xml = GetXmlHeader();
//	xml = xml + Workbook->getXML();
	FILE *file = HandleFileExport(filename);
	if (file)
	{
		fprintf( file,"%s",xml.c_str() );
		Workbook->saveToFile(filename, 8*1024/*chunksize*/,file);
		fclose(file);
	}
}


//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportDependenciesToFile( CResourceCollector &stats,const char *filename )
{
	XmlNodeRef Workbook = GetISystem()->CreateXmlNode("Workbook");

	{
		InitExcelWorkbook(Workbook);
		ExportDependencies(stats);
	}

	string xml = GetXmlHeader();
//	xml = xml + Workbook->getXML();

	FILE *file = HandleFileExport(filename);
	if (file)
	{
		fprintf( file,"%s",xml.c_str() );
		Workbook->saveToFile(filename, 8*1024/*chunksize*/,file);
		fclose(file);
	}
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::Export( XmlNodeRef Workbook,SCryEngineStats &stats )
{
	InitExcelWorkbook(Workbook);
	ExportSummary(stats);
	ExportMemInfo(stats);
	ExportMemStats(stats);
	ExportStatObjects(stats);
	ExportCharacters(stats);
	ExportRenderMeshes(stats);
	ExportTextures(stats);
  ExportMaterials( stats );
	ExportWavebanks(stats);
	ExportSoundInfos(stats);
	ExportTimeDemoInfo();
	ExportProfilerStatistics(stats);
	ExportAnimationStatistics(stats);
	ExportAllLoadingStatistics(stats);
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportSummary( SCryEngineStats &stats )
{
	// Make summary sheet.en
	NewWorksheet( "Summary" );

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",200 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );

	// get version
	const SFileVersion & ver = GetISystem()->GetFileVersion();
	char sVersion[128];
	ver.ToString(sVersion);
	string levelName = "no_level";
	ICVar *sv_map = gEnv->pConsole->GetCVar("sv_map");
	if (sv_map)
		levelName = sv_map->GetString();

	AddRow();
	AddCell( string("CryEngine Version ")+sVersion );
	AddRow();
	AddCell( string("Level ")+levelName );
	AddRow();
#ifdef WIN64
	AddCell( "Running in 64bit version" );
#elif defined(PS3)
	AddCell( "Running PS3 version" );
#else
	AddCell( "Running in 32bit version" );
#endif
	AddRow();
	AddCell( "Level Load Time (sec):" );
	AddCell( (int)stats.fLevelLoadTime );
	AddRow();
	AddRow();
	AddCell( "Average\\Min\\Max (fps):" );
	AddCell( (int)stats.infoFPS.fAverageFPS );
	AddCell( (int)stats.infoFPS.fMinFPS );
	AddCell( (int)stats.infoFPS.fMaxFPS );
	AddRow();

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Resource Type (MB)" );
	AddCell( "Count" );
	AddCell( "Memory Size" );
	AddCell( "Only Mesh Size" );
	AddCell( "Only Texture Size" );

	AddRow();
	AddCell( "CGF Objects",CELL_BOLD );
	AddCell( stats.objects.size() );
	AddCell( (stats.nStatObj_SummaryTextureSize+stats.nStatObj_SummaryMeshSize)/(1024*1024) );
	AddCell( (stats.nStatObj_SummaryMeshSize)/(1024*1024) );
	AddCell( (stats.nStatObj_SummaryTextureSize)/(1024*1024) );
	AddRow();
	AddCell( "Character Models",CELL_BOLD );
	AddCell( stats.characters.size() );
	AddCell( (stats.nChar_SummaryTextureSize+stats.nChar_SummaryMeshSize)/(1024*1024) );
	AddCell( (stats.nChar_SummaryMeshSize)/(1024*1024) );
	AddCell( (stats.nChar_SummaryTextureSize)/(1024*1024) );


	AddRow();
	AddCell( "Character Instances",CELL_BOLD );
	AddCell( stats.nChar_NumInstances );

	//AddCell( stats.nMemCharactersTotalSize );
	AddRow();
	
	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Textures" );
	AddRow();
	AddCell( "Count",CELL_BOLD );
	AddCell( stats.textues.size() );
	AddCell( "Total count of textures");
	AddRow();

	AddCell( "Textures Overall Size",CELL_BOLD );
	AddCell( stats.nSummary_TextureSize/(1024*1024) );
	AddCell( "Total amount of textures memory usage");

	AddRow();
	AddCell( "Pool Size",CELL_BOLD );
	AddCell( stats.nSummary_TexturesPoolSize / 1024 / 1024 );
	AddCell( "Size of textures pool");

	AddRow();
	AddCell( "Textures Memory Usage",CELL_BOLD );
	AddCell( (stats.nSummary_TexturesPoolSize + stats.nSummary_UserTextureSize + stats.nSummary_EngineTextureSize )/(1024*1024) );
	AddCell( "Total memory of textures in RAM");

	AddRow();
	AddCell( "Textures Engine Only",CELL_BOLD );
	AddCell( (stats.nSummary_EngineTextureSize)/(1024*1024) );
	AddCell( "Textures for internal Engine usage ");

	AddRow();
	AddCell( "User Textures",CELL_BOLD );
	AddCell( (stats.nSummary_UserTextureSize)/(1024*1024) );
	AddCell( "User textures not stored in the textures pool");

	AddRow();
	AddCell( "Textures streaming throughput(KB/s)",CELL_BOLD );;
	if(stats.nSummary_TexturesStreamingThroughput > 0)
		AddCell( (stats.nSummary_TexturesStreamingThroughput) / 1024 );
	AddRow();

	//AddRow();
	//m_CurrRow->setAttr( "ss:StyleID","s25" );
	//AddCell( "Resource Type (MB)" );
	//AddCell( "Count" );
	//AddCell( "Total Size" );
	//AddCell( "System Memory" );
	//AddCell( "Video Memory" );
	//AddCell( "Engine Textures" );
	//AddCell( "User Textures" );
	//if(stats.nSummary_TexturesStreamingThroughput > 0)
	//	AddCell( "Textures streaming throughput(KB/s)" );

	//AddRow();
	//AddCell( "Textures",CELL_BOLD );
	//AddCell( stats.textues.size() );
	//AddCell( (stats.nSummary_TextureSize)/(1024*1024) );
	//AddCell( (stats.nSummary_TextureSize)/(1024*1024) );
	//AddCell( (stats.nSummary_TextureSize)/(1024*1024) );
	//AddCell( (stats.nSummary_EngineTextureSize)/(1024*1024) );
	//AddCell( (stats.nSummary_UserTextureSize)/(1024*1024) );
	//if(stats.nSummary_TexturesStreamingThroughput > 0)
	//	AddCell( (stats.nSummary_TexturesStreamingThroughput) / 1024 );
	
	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Meshes");
	AddRow();
	AddCell( "Count",CELL_BOLD );
	AddCell( stats.nSummaryMeshCount );
	AddRow();
	AddCell( "Total Size", CELL_BOLD );
	AddCell( (stats.nAPI_MeshSize+stats.nSummaryMeshSize)/(1024*1024) );
	AddRow();
	AddCell( "System Memory", CELL_BOLD );
	AddCell( (stats.nSummaryMeshSize)/(1024*1024) );
	AddRow();
	AddCell( "Video Memory", CELL_BOLD );
	AddCell( (stats.nAPI_MeshSize)/(1024*1024) );
	AddRow();
	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddRow();

	AddRow();
	AddCell( "Lua Memory Usage (MB)" );
	AddCell( stats.nSummaryScriptSize/(1024*1024) );

	AddRow();
	AddCell( "Game Memory Usage (MB)" );
#ifdef PS3
	//use currently used allocated heap memory for this
	malloc_managed_size ps3_heap_stats;
	malloc_stats(&ps3_heap_stats);
	AddCell( ps3_heap_stats.current_inuse_size/(1024*1024) );
#else
	AddCell( stats.nTotalAllocatedMemory/(1024*1024) );
#endif
	uint64 totalAll = stats.memInfo.totalUsedInModules;
	totalAll += stats.nAPI_MeshSize;
	totalAll += stats.nSummary_UserTextureSize;
	totalAll += stats.nSummary_EngineTextureSize;
	totalAll += stats.memInfo.totalCodeAndStatic;
	AddRow();
	AddCell( "Total Allocated (With Code/Textures/Mesh) (MB)" );
#ifdef PS3
	uint32 peakMemUsage = ps3_heap_stats.max_system_size/(1024*1024) + stats.memInfo.totalCodeAndStatic/(1024*1024);
	AddCell( peakMemUsage );
#else
	AddCell( totalAll/(1024*1024) );
#endif
	AddRow(); 
	AddRow();
	AddCell( "Virtual Memory Usage (MB)" );
#if defined(PS3)
	AddCell( ps3_heap_stats.current_inuse_size/(1024*1024) + stats.memInfo.totalCodeAndStatic/(1024*1024) );
#else
	AddCell( stats.nWin32_PagefileUsage/(1024*1024) );
#endif
	AddRow();
	AddCell( "Peak Virtual Memory Usage (MB)" );
#if defined(PS3)
	AddCell( peakMemUsage );
	AddRow();
	AddCell( "GPU Memory Usage (MB)" );
	size_t VidMem=0,LastVidMem=0;
	gEnv->pRenderer->GetVideoMemoryUsageStats(VidMem,LastVidMem);
	AddCell( VidMem );
#else
	AddCell( stats.nWin32_PeakPagefileUsage/(1024*1024) );
#endif
	//FPS buckets
	ExportFPSBuckets();
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportStatObjects( SCryEngineStats &stats )
{
	NewWorksheet( "Static Geometry" );
	FreezeFirstRow();
	AutoFilter(1,12);

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",350 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",60 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",60 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",120 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Filename" );
	AddCell( "Refs" );
	AddCell( "Mesh Size (KB)" );
	AddCell( "Texture Size (KB)" );
	AddCell( "LODs" );
	AddCell( "Sub Meshes" );
	AddCell( "Vertices" );
	AddCell( "Tris" );
	AddCell( "Physics Tris" );
	AddCell( "Physics Size (KB)" );
	AddCell( "LODs Tris" );
	AddCell( "Mesh Size Loaded (KB)" );
	AddCell( "Split LODs" );

	int nRows = (int)stats.objects.size();
	for (int i = 0; i < nRows; i++)
	{
		SCryEngineStats::StatObjInfo &si = stats.objects[i];

		AddRow();
		AddCell( (si.pStatObj) ? si.pStatObj->GetFilePath() : "" );
		AddCell( si.nNumRefs );
		AddCell( si.nMeshSize/1024 );
		AddCell( si.nTextureSize/1024 );
		AddCell( si.nLods );
		AddCell( si.nSubMeshCount );
		AddCell( si.nVertices );
		AddCell( si.nIndices/3 );
		AddCell( si.nPhysPrimitives );
		AddCell( (si.nPhysProxySize+512)/1024 );

		if (si.nLods > 1)
		{
			// Print lod1/lod2/lod3 ...
			char tempstr[256];
			char numstr[32];
			tempstr[0] = 0;
			int numlods = 0;
			for (int lod = 0; lod < MAX_LODS; lod++)
			{
				if (si.nIndicesPerLod[lod] != 0)
				{
					sprintf_s(numstr,sizeof(numstr),"%d",(si.nIndicesPerLod[lod]/3) );
					if (numlods > 0)
						strcat_s(tempstr,sizeof(tempstr)," / ");
					strcat_s(tempstr,sizeof(tempstr),numstr);
					numlods++;
				}
			}
			if (numlods > 1)
				AddCell(tempstr);
		}
		else
		{
			AddCell("");
		}

		AddCell( si.nMeshSizeLoaded/1024 );
		if (si.bSplitLods)
		{
			AddCell( "Yes" );
		}
		else
		{
			AddCell( "" );
		}
	}

	/*
	AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "" );
	AddCell_SumOfRows( nRows ); // Mesh size
	AddCell_SumOfRows( nRows ); // Texture size
	AddCell( "" ); // LODs
	AddCell( "" ); // Sub Meshes
	AddCell_SumOfRows( nRows ); // Vertices
	AddCell_SumOfRows( nRows ); // Tris
	AddCell_SumOfRows( nRows ); // Physics Tris
	AddCell_SumOfRows( nRows ); // Physics Size
	AddCell( "" ); // LODs Tris
	AddCell_SumOfRows( nRows ); // Mesh Size Loaded
	*/
}


//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportAllLoadingStatistics( SCryEngineStats &stats )
{
	ExportLoadingStatistics(stats);
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportLoadingStatistics( SCryEngineStats &stats)
{
	NewWorksheet( "Load Stats" );

	FreezeFirstRow();

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",300 );
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Function" );
	AddCell( "Self (sec)" );
	AddCell( "Total (sec)" );
	AddCell( "Calls" );
	AddCell( "Memory (Mb)" );
	AddCell( "Read file size" );
	AddCell( "Bandwith self (Kb/s)" );
	AddCell( "Bandwith total (Kb/s)" );
	AddCell( "FOpens self" );
	AddCell( "FReads self" );
	AddCell( "FSeeks self" );
	AddCell( "FOpens total" );
	AddCell( "FReads total" );
	AddCell( "FSeeks total" );

	int nRows = (int)stats.loading.size();
	for (int i = 0; i < nRows; i++)
	{
		SLoadingProfilerInfo &an = stats.loading[i];
		AddRow();
		AddCell( an.name );
		AddCell( (float)an.selfTime );
		AddCell( (float)an.totalTime );
		AddCell( an.callsTotal );
		AddCell( (float)an.memorySize );
		AddCell( (float)an.selfInfo.m_dOperationSize / 1024.0f );
		float bandwithSelf = an.selfTime > 0. ? (float)(an.selfInfo.m_dOperationSize / an.selfTime / 1024.0 ) : 0.0f;
		float bandwithTotal = an.totalTime > 0. ? (float)(an.totalInfo.m_dOperationSize / an.totalTime / 1024.0 ) : 0.0f;
		AddCell( bandwithSelf);
		AddCell( bandwithTotal);
		AddCell( an.selfInfo.m_nFileOpenCount);
		AddCell( an.selfInfo.m_nFileReadCount);
		AddCell( an.selfInfo.m_nSeeksCount);
		AddCell( an.totalInfo.m_nFileOpenCount);
		AddCell( an.totalInfo.m_nFileReadCount);
		AddCell( an.totalInfo.m_nSeeksCount);

	}
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportAnimationStatistics( SCryEngineStats &stats )
{
	if (stats.animations.empty())
		return;

	NewWorksheet( "Animations" );
	FreezeFirstRow();
	AutoFilter(1,2);

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",300 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Name" );
	AddCell( "Count" );

	int nRows = (int)stats.animations.size();
	for (int i = 0; i < nRows; i++)
	{
		SAnimationStatistics &an = stats.animations[i];
		AddRow();
		AddCell( an.name );
		AddCell( (uint32)an.count );
	}
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportProfilerStatistics( SCryEngineStats &stats )
{
	if (stats.profilers.empty())
		return;

	{

	NewWorksheet( "Profiler" );
	FreezeFirstRow();
	AutoFilter(1,10);

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",300 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );

//	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",40 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Module" );
	AddCell( "Name" );
	AddCell( "Self time, ms" );
	AddCell( "Total time, ms" );
	AddCell( "Count" );
	AddCell( "" );
	AddCell( "Min time, ms" );
	AddCell( "Max time, ms" );
	AddCell( "Min count" );
	AddCell( "Max count" );

	int nRows = (int)stats.profilers.size();
	for (int i = 0; i < nRows; i++)
	{
		SCryEngineStats::ProfilerInfo &pi = stats.profilers[i];
		AddRow();
		AddCell( pi.m_module );
		AddCell( pi.m_name );
		AddCell( pi.m_displayedValue );
		AddCell( pi.m_totalTime );
		AddCell( pi.m_count );
		AddCell( "");
		AddCell( pi.m_min );
		AddCell( pi.m_max );
		AddCell( pi.m_mincount );
		AddCell( pi.m_maxcount );

	}

	AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell("");
	AddCell("");
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
//	AddCell("");
	AddCell("");
	AddCell("");
	}

	// Peaks

	NewWorksheet( "Peaks" );

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",300 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	//Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	//Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	//Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	//Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	//Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	//Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
	//Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );

	//	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",40 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Module" );
	AddCell( "Name" );
	AddCell( "Peak, ms" );
	//AddCell( "Self time, ms" );
	//AddCell( "Total time, ms" );
	AddCell( "Count" );
	//AddCell( "" );
	//AddCell( "Min time, ms" );
	//AddCell( "Max time, ms" );
	//AddCell( "Min count" );
	//AddCell( "Max count" );

	int nRows = (int)stats.peaks.size();
	for (int i = 0; i < nRows; i++)
	{
		SCryEngineStats::SPeakProfilerInfo &peak = stats.peaks[i];
		SCryEngineStats::ProfilerInfo &pi = stats.peaks[i].profiler;
		AddRow();
		AddCell( pi.m_module );
		AddCell( pi.m_name );
		AddCell( peak.peakValue );
		//AddCell( pi.m_displayedValue );
		//AddCell( pi.m_totalTime );
		AddCell( peak.count );
		//AddCell( "");
		//AddCell( pi.m_min );
		//AddCell( pi.m_max );
		//AddCell( pi.m_mincount );
		//AddCell( pi.m_maxcount );
	}

	//AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
	//AddCell("");
	//AddCell("");
	//AddCell("");
	//AddCell_SumOfRows( nRows );
	//AddCell_SumOfRows( nRows );
	//AddCell_SumOfRows( nRows );
	////	AddCell("");
	//AddCell("");
	//AddCell("");

	NewWorksheet( "Budget" );
	{
		Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",100 );
		Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",300 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",50 );

		AddRow();
		m_CurrRow->setAttr( "ss:StyleID","s25" );
		AddCell( "Module" );
		AddCell( "OverBudgetRatio%" );

		nRows = (int)stats.moduleprofilers.size();
		for (int i = 0; i < nRows; i++)
		{
			SCryEngineStats::SModuleProfilerInfo &moduleProfile = stats.moduleprofilers[i];
			AddRow();
			AddCell( moduleProfile.name );
			AddCell( moduleProfile.overBugetRatio*100 );
		}
	}

}


//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportCharacters( SCryEngineStats &stats )
{
	NewWorksheet( "Characters" );
	FreezeFirstRow();
	AutoFilter(1,10);

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",300 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",40 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",40 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",150 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Filename" );
	AddCell( "Num Instances" );
	AddCell( "Mesh Size (KB)" );
	AddCell( "Texture Size (KB)" );
	AddCell( "PhysProxy Size (KB)" );
	AddCell( "LODs" );
	AddCell( "Vertices Lod0" );
	AddCell( "Tris Lod0" );
	AddCell( "All Vertices" );
	AddCell( "All Tris" );
	AddCell( "LOD Tris" );
	
	int nRows = (int)stats.characters.size();
	for (int i = 0; i < nRows; i++)
	{
		SCryEngineStats::CharacterInfo &si = stats.characters[i];
		AddRow();
		AddCell( si.pModel->GetModelFilePath() );
		AddCell( si.nInstances );
		AddCell( si.nMeshSize/1024 );
		AddCell( si.nTextureSize/1024 );
		AddCell( (si.nPhysProxySize+512)>>10 );
		AddCell( si.nLods );
		AddCell( si.nVertices );
		AddCell( si.nIndices/3 );

		int nAllVerts = 0;
		for (int k = 0; k < MAX_LODS; k++) nAllVerts += si.nVerticesPerLod[k];
		AddCell( nAllVerts );

		int nAllIndices = 0;
		for (int k = 0; k < MAX_LODS; k++) nAllIndices += si.nIndicesPerLod[k];
		AddCell( nAllIndices/3 );

		if (si.nLods > 1)
		{
			// Print lod1/lod2/lod3 ...
			char tempstr[256];
			char numstr[32];
			tempstr[0] = 0;
			int numlods = 0;
			for (int lod = 0; lod < MAX_LODS; lod++)
			{
				if (si.nIndicesPerLod[lod] != 0)
				{
					sprintf_s(numstr,sizeof(numstr),"%d",(si.nIndicesPerLod[lod]/3) );
					if (numlods > 0)
						strcat_s(tempstr,sizeof(tempstr)," / ");
					strcat_s(tempstr,sizeof(tempstr),numstr);
					numlods++;
				}
			}
			if (numlods > 1)
				AddCell(tempstr);
		}
	}
	AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell("");
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
	AddCell("");
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportDependencies( CResourceCollector &stats )
{
	{
		NewWorksheet( "Assets" );
		FreezeFirstRow();

		XmlNodeRef Column;
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",72 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",108 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",73 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",60 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",1000 );

		AddRow();
		m_CurrRow->setAttr( "ss:StyleID","s25" );
		AddCell( "Instances" );
		AddCell( "Dependencies" );
		AddCell( "FileMem (KB)" );
		AddCell( "Extension" );
		AddCell( "FileName" );

		std::vector<CResourceCollector::SAssetEntry>::const_iterator it, end=stats.m_Assets.end();
		uint32 dwAssetID=0;

		for(it=stats.m_Assets.begin();it!=end;++it,++dwAssetID)
		{
			const CResourceCollector::SAssetEntry &rRef = *it;

			AddRow();

			char szAName1[1024];		sprintf_s(szAName1,"A%d %s",dwAssetID,rRef.m_sFileName.c_str());

			AddCell( rRef.m_dwInstanceCnt );
			AddCell( rRef.m_dwDependencyCnt );
			AddCell( rRef.m_dwFileSize!=0xffffffff ? rRef.m_dwFileSize/1024 : 0 );
			AddCell( PathUtil::GetExt(rRef.m_sFileName) );
			AddCell( szAName1 );
		}

		AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
		AddCell_SumOfRows( dwAssetID );
		AddCell("");
		AddCell_SumOfRows( dwAssetID );
	}

	{
		NewWorksheet( "Dependencies" );
		FreezeFirstRow();
		
		XmlNodeRef Column;
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",600 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",60 );

		AddRow();
		m_CurrRow->setAttr( "ss:StyleID","s25" );
		AddCell( "Asset Filename" );
		AddCell( "Requires Sum (KB)" );
		AddCell( "Requires Count" );

		std::set<CResourceCollector::SDependencyPair>::const_iterator it, end=stats.m_Dependencies.end();

		uint32 dwCurrentAssetID=0xffffffff;
		uint32 dwSumFile=0,dwSum=0;

		for(it=stats.m_Dependencies.begin();;++it)
		{
			uint32 dwAssetsSize = stats.m_Assets.size();

			if(it==end || (*it).m_idAsset!=dwCurrentAssetID)
			{
				if(dwSum!=0 && dwSumFile!=0xffffffff)
				{
					assert(dwCurrentAssetID<dwAssetsSize);

					char szAName0[1024];		sprintf_s(szAName0,"A%d %s",dwCurrentAssetID,stats.m_Assets[dwCurrentAssetID].m_sFileName.c_str());

					AddRow();
					AddCell( szAName0 );
					AddCell( dwSumFile/1024 );
					AddCell( dwSum );
				}

				dwSumFile=0;dwSum=0;

				if(it==end)
					break;
			}

			const CResourceCollector::SDependencyPair &rRef = *it;		

			assert(rRef.m_idDependsOnAsset<dwAssetsSize);

			CResourceCollector::SAssetEntry &rDepAsset = stats.m_Assets[rRef.m_idDependsOnAsset];

			if(rDepAsset.m_dwFileSize!=0xffffffff)
				dwSumFile += rDepAsset.m_dwFileSize;

			++dwSum;

			dwCurrentAssetID=rRef.m_idAsset;
		}
	}


	{
		NewWorksheet( "Detailed Dependencies" );
		FreezeFirstRow();

		XmlNodeRef Column;
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",600 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",1000 );
		Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",70 );

		AddRow();
		m_CurrRow->setAttr( "ss:StyleID","s25" );
		AddCell( "Asset Filename" );
		AddCell( "Requires Filename" );
		AddCell( "Requires (KB)" );

		std::set<CResourceCollector::SDependencyPair>::const_iterator it, end=stats.m_Dependencies.end();

		uint32 dwCurrentAssetID=0xffffffff;
		uint32 dwSumFile=0,dwSum=0;

		for(it=stats.m_Dependencies.begin();;++it)
		{
			if(it==end || (*it).m_idAsset!=dwCurrentAssetID)
			{
				if(dwSum!=0 && dwSumFile!=0xffffffff)
				{
					//					AddRow(); m_CurrRow->setAttr( "ss:StyleID","s21" );
					//					AddCell("");
					//					AddCell_SumOfRows( dwSum );
					AddRow();
					AddCell( "" );
				}

				dwSumFile=0;dwSum=0;

				if(it==end)
					break;
				/*
				const CResourceCollector::SDependencyPair &rRef = *it;		

				char szAName0[20];		sprintf_s(szAName0,"A%d",rRef.m_idAsset);

				AddRow(); m_CurrRow->setAttr( "ss:StyleID","s21" );
				AddCell( szAName0 );
				AddCell( stats.m_Assets[rRef.m_idAsset].m_sFileName.c_str() );
				*/
			}

			const CResourceCollector::SDependencyPair &rRef = *it;		

			CResourceCollector::SAssetEntry &rDepAsset = stats.m_Assets[rRef.m_idDependsOnAsset];

			AddRow();

			char szAName0[1024];		sprintf_s(szAName0,"A%d %s",rRef.m_idAsset,stats.m_Assets[rRef.m_idAsset].m_sFileName.c_str());
			char szAName1[1024];		sprintf_s(szAName1,"A%d %s",rRef.m_idDependsOnAsset,rDepAsset.m_sFileName.c_str());

			AddCell( szAName0 );
			AddCell( szAName1 );
			AddCell( rDepAsset.m_dwFileSize!=0xffffffff ? rDepAsset.m_dwFileSize/1024 : 0 );

			if(rDepAsset.m_dwFileSize!=0xffffffff)
				dwSumFile += rDepAsset.m_dwFileSize;

			++dwSum;

			dwCurrentAssetID=rRef.m_idAsset;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportRenderMeshes( SCryEngineStats &stats )
{
	NewWorksheet( "Meshes" );
	FreezeFirstRow();
	AutoFilter(1,8);

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",300 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",40 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
  Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",100 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Mesh Type" );
	AddCell( "Num Instances" );
	AddCell( "Mesh Size Sys (KB)" );
  AddCell( "Mesh Size Dev (KB)" );
	AddCell( "Texture Size (KB)" );
	AddCell( "Total Vertices" );
	AddCell( "Total Tris" );
	int nRows = (int)stats.meshes.size();
	for (int i = 0; i < nRows; i++)
	{
		SCryEngineStats::MeshInfo &mi = stats.meshes[i];
		AddRow();
		AddCell( mi.name );
		AddCell( mi.nCount );
		AddCell( mi.nMeshSizeSys/1024 );
    AddCell( mi.nMeshSizeDev/1024 );
		AddCell( mi.nTextureSize/1024 );
		AddCell( mi.nVerticesSum );
		AddCell( mi.nIndicesSum/3 );
	}
	AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell("");
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportMaterials( SCryEngineStats &stats )
{
  NewWorksheet( "Materials" );
	FreezeFirstRow();
	AutoFilter(1,5);

  XmlNodeRef Column;
  Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",400 );

  AddRow();
  m_CurrRow->setAttr( "ss:StyleID","s25" );

  AddCell( "Name" );
  AddCell( "System?" );
  AddCell( "Ref counter" );
  AddCell( "Childs num" );

  int nRows = (int)stats.materials.size();
  //char texres[32];
  for (int i = 0; i < nRows; i++)
  {
    IMaterial *pMat = stats.materials[i];
    AddRow();
    AddCell( pMat->GetName() );

    if(pMat->GetShaderItem().m_pShader && pMat->GetShaderItem().m_pShader->GetFlags()&EF_SYSTEM)
      AddCell( "YES" );
    else
      AddCell( "NO" );

    AddCell( pMat->GetNumRefs() );

    AddCell( pMat->GetSubMtlCount() );
  }

//  AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
  //AddCell("");
  //AddCell_SumOfRows( nRows );
}

void CStatsToExcelExporter::ExportTextureAllocations()
{
#if defined(PS3) && defined(MEM_MAN_ADD_SIZE_BLOCK_VMEM)
	//create tab with detailed RSX allocation
  int32 Sum[4]={0,0,0,0};
  int32* pPS3MemStats = (int*) GetISystem()->GetIRenderer()->EF_Query( EFQ_PS3_Resource_Stats );

	NewWorksheet( "VRAM Resources" );

	FreezeFirstRow();
	AutoFilter(1,6);

  XmlNodeRef Column;
  Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",120 );
  Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",100 );
  Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",90 );
  Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",100 );
  Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",90 );

  AddRow();
  m_CurrRow->setAttr( "ss:StyleID","s25" );

  AddCell( "Resourcename" );
  AddCell( "Allocated elements" );
  AddCell( "Main Memory" );
  AddCell( "VRam requested" );
  AddCell( "VRam allocated" );

  const uint32 nRows = 21;
	const char* pNames[nRows] = 
		{"SwapChain","Blob","Buffer","PS Cache Buffer","Vertex Buffer","Index Buffer",
		"Const Buffer","SamplerState","BlendState","RasterizerState","DepthStencilState","Texture1D",
		"Texture2D","Engine Texture2D","Texture3D","RenderTargetView","DepthStencilView","ShaderResourceView","Shader",
		"ShaderReflection","InputLayout"};
  for (uint32 i = 0; i < nRows; i++)
  {
		AddRow();
		AddCell(pNames[i]);
		AddCell(pPS3MemStats[0]);
		AddCell(pPS3MemStats[1]/1024);
		AddCell(pPS3MemStats[2]/1024);
		AddCell(pPS3MemStats[3]/1024);
		Sum[0]+=pPS3MemStats[0];
		Sum[1]+=pPS3MemStats[1]/1024;
		Sum[2]+=pPS3MemStats[2]/1024;
		Sum[3]+=pPS3MemStats[3]/1024;
		pPS3MemStats+=4;
	}
	AddRow();
	AddRow();
  m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell("");
	AddCell(Sum[0]);	AddCell(Sum[1]);	AddCell(Sum[2]);	AddCell(Sum[3]);
#endif
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportTextures( SCryEngineStats &stats )
{
	ExportTextureAllocations();
	NewWorksheet( "Textures" );
	FreezeFirstRow();
	AutoFilter(1,10);

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",400 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",40 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",40 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	
	AddCell( "Filename" );
	AddCell( "Texture Size (KB)" );
	AddCell( "Resolution" );
	AddCell( "Mip Levels" );
	AddCell( "Type" );
	AddCell( "Format" );
	AddCell( "Usage" );
	AddCell( "Actual current size (KB)" );
	AddCell( "Last frame used" );

	int nRows = (int)stats.textues.size();
	for (int i = 0; i < nRows; i++)
	{
		ITexture *pTexture = stats.textues[i];
		AddRow();
		AddCell( pTexture->GetName() );
		AddCell( pTexture->GetDataSize()/1024 );
		{
			char texres[128];
			sprintf_s( texres,"%d x %d",pTexture->GetWidth(),pTexture->GetHeight() );
			AddCell( texres,CELL_CENTERED );
		}
		AddCell( pTexture->GetNumMips() );
		AddCell( pTexture->GetTypeName(),CELL_CENTERED );
		AddCell( pTexture->GetFormatName(),CELL_CENTERED );
		if (pTexture->IsStreamedVirtual())
			AddCell("Streamed", CELL_CENTERED);
		else
		{
			const char* pTexDesc = "Static";
			uint32 texFlags = pTexture->GetFlags();
			if (texFlags & FT_USAGE_RENDERTARGET)
				pTexDesc = "Render Target";
			else if (texFlags & FT_USAGE_DYNAMIC)
				pTexDesc = "Dynamic";
			else if (texFlags & FT_USAGE_ATLAS)
				pTexDesc = "Atlas";
			AddCell(pTexDesc, CELL_CENTERED);
		}
		AddCell( pTexture->GetDeviceDataSize()/1024 );
		{
			char pTexDesc[16];
			const uint32 nFrameId = pTexture->GetAccessFrameId();
			if(nFrameId == -1)
				sprintf_s(pTexDesc, "Not used");
			else
				sprintf_s(pTexDesc, "%d", nFrameId);
			AddCell( pTexDesc, CELL_CENTERED);
		}
	}

	AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell("");
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportWavebanks( SCryEngineStats &stats )
{
	if (stats.wavebanks.empty())
		return;

	NewWorksheet( "Wavebanks" );
	FreezeFirstRow();
	AutoFilter(1,6);

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",400 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",40 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );

	AddCell( "Filename" );
	AddCell( "Wavebank Size (KB)" );
	AddCell( "Times Used" );
	AddCell( "Current Memory (KB)" );
	AddCell( "Peak Memory (KB)" );

	int nRows = (int)stats.wavebanks.size();
	for (int i = 0; i < nRows; i++)
	{
		IWavebank *pWavebank = stats.wavebanks[i];
		AddRow();
		
		string sFullName = pWavebank->GetPath();
		sFullName += pWavebank->GetName();
		sFullName += ".fsb";

		AddCell( sFullName );
		AddCell( pWavebank->GetInfo()->nFileSize/1024 );
		AddCell (pWavebank->GetInfo()->nTimesAccessed );
		AddCell (pWavebank->GetInfo()->nMemCurrentlyInByte/1024 );
		AddCell (pWavebank->GetInfo()->nMemPeakInByte/1024 );
	}

	AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell("");
	AddCell_SumOfRows( nRows );
	AddCell("");
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportSoundInfos( SCryEngineStats &stats )
{
	if (stats.soundinfos.empty())
		return;

	NewWorksheet( "SoundInfos" );
	FreezeFirstRow();
	AutoFilter(1,8);

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",400 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",70 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",120 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );

	AddCell( "Soundname" );
	AddCell( "Times Played" );
	AddCell( "Times Played on a Channel" );
	AddCell( "Memory Size (KB)" );
	AddCell( "Peak Spawns" );
	AddCell( "Channels Used" );

	int nRows = (int)stats.soundinfos.size();
	for (int i = 0; i < nRows; i++)
	{
		ISoundProfileInfo *pSoundInfo = stats.soundinfos[i];
		AddRow();
		AddCell( pSoundInfo->GetName() );
		AddCell (pSoundInfo->GetInfo()->nTimesPlayed );
		AddCell (pSoundInfo->GetInfo()->nTimesPlayedOnChannel );
		AddCell( pSoundInfo->GetInfo()->nMemorySize/1024 );
		AddCell (pSoundInfo->GetInfo()->nPeakSpawn );
		AddCell (pSoundInfo->GetInfo()->nChannelsUsed );
	}

	AddRow(); m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell("");
	AddCell_SumOfRows( nRows );
	AddCell("");
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportMemStats(SCryEngineStats &stats )
{
	if (!stats.memInfo.m_pStats)
		return;

	NewWorksheet( "Memory Stats" );
	FreezeFirstRow();

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",300 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );

	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	
	AddCell( "Section" );
	AddCell( "Size (KB)" );
	AddCell( "Total Size (KB)" );
	AddCell( "Object Count" );

	AddRow();

	for (unsigned i = 0; i < stats.memInfo.m_pStats->size(); ++i)
	{
		AddRow();
		const CrySizerStats::Component& rComp = (*stats.memInfo.m_pStats)[i];

		if (rComp.nDepth < 2)
		{
			AddRow(); // Skip one row if primary component.
			m_CurrRow->setAttr( "ss:StyleID","s25" );
		}

		char szDepth[64] = "                                                              ";
		if (rComp.nDepth < sizeof(szDepth))
			szDepth[rComp.nDepth*2] = '\0';

		string sCompDisplayName = szDepth;
		sCompDisplayName += rComp.strName;
		
		AddCell( sCompDisplayName.c_str() );
		//char szSize[32];
		//sprintf(szSize, "%s%7.3f", szDepth,rComp.getSizeMBytes() );
		//AddCell( szSize );
		//sprintf(szSize, "%s%7.3f", szDepth,rComp.getTotalSizeMBytes() );
		//AddCell( szSize );

		if (rComp.sizeBytes > 0)
			AddCell( (unsigned int)(rComp.sizeBytes/1024) );
		else
			AddCell( "" );
		AddCell( (unsigned int)(rComp.sizeBytesTotal/1024) );

		if (rComp.numObjects > 0)
		{
			AddCell( (unsigned int)(rComp.numObjects) );
		}

		//if (rComp.sizeBytesTotal <= m_nMinSubcomponentBytes || rComp.nDepth > m_nMaxSubcomponentDepth)
			//continue;



		/*
		char szDepth[32] = " ..............................";
		if (rComp.nDepth < sizeof(szDepth))
			szDepth[rComp.nDepth] = '\0';

		char szSize[32];
		if (rComp.sizeBytes > 0)
		{
			if (rComp.sizeBytesTotal > rComp.sizeBytes)
				sprintf (szSize, "%7.3f  %7.3f", rComp.getTotalSizeMBytes(), rComp.getSizeMBytes());
			else
				sprintf (szSize, "         %7.3f", rComp.getSizeMBytes());
		}
		else
		{
			assert (rComp.sizeBytesTotal > 0);
			sprintf (szSize, "%7.3f         ", rComp.getTotalSizeMBytes());
		}
		char szCount[16];

		if (rComp.numObjects)
			sprintf (szCount, "%8u", rComp.numObjects);
		else
			szCount[0] = '\0';

		m_pLog->LogToFile ("%s%-*s:%s%s",szDepth, nNameWidth-rComp.nDepth,rComp.strName.c_str(), szSize, szCount);
		*/
	}

	//delete m_pStats;
	//delete m_pSizer;
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportMemInfo( SCryEngineStats &stats )
{
	NewWorksheet( "Modules Memory Info" );
	FreezeFirstRow();

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",300 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
#if !defined(PS3) || (defined(PS3) && !defined(_LIB))
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",20 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",90 );
#endif
	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Module" );
	AddCell( "Dynamic(KB)" );
	AddCell( "Num Allocs" );
	AddCell( "Sum Of Allocs (KB)" );
#if !defined(PS3)
	AddCell( "" );
	AddCell( "Static Total (KB)" );
	AddCell( "Static Code (KB)" );
	AddCell( "Static Init. Data (KB)" );
	AddCell( "Static Uninit. Data (KB)" );
	AddCell( "Strings (KB)" );
	AddCell( "STL (KB)" );
	AddCell( "STL Wasted (KB)" );
	AddCell( "Dynamic - Wasted (KB)" );
#endif
	AddRow();

#if defined(PS3)
  MTrace::SystemStats sys_stats; 
  MTrace::SystemStatistics(sys_stats); 
	const std::vector<const char*>& szModules = GetModuleNames();
	stats.memInfo.totalUsedInModules = 0;
#endif

	//////////////////////////////////////////////////////////////////////////
	int nRows = 0;

	for (uint32 i = 0; i < stats.memInfo.modules.size(); i++)
	{
#if defined(PS3) 
		const char *szModule = szModules[i];
		SCryEngineStatsModuleInfo moduleInfo;
		//fetch values from mtrace or set to 0 as for _LIB compilation nothing useful is available
	#if defined(SUPP_MTRACE)
    moduleInfo.memInfo.allocated				= sys_stats.modules[(ECryModule)i].allocated; 
    moduleInfo.memInfo.freed						= sys_stats.modules[(ECryModule)i].freed;
    moduleInfo.memInfo.num_allocations	= sys_stats.modules[(ECryModule)i].num_allocations;
		moduleInfo.memInfo.requested				= 
    moduleInfo.usedInModule							= sys_stats.modules[(ECryModule)i].current_memory;
		stats.memInfo.totalUsedInModules	 += moduleInfo.usedInModule;
	#else
		moduleInfo.usedInModule = 0;	memset(&moduleInfo.memInfo,0,sizeof(moduleInfo.memInfo));
	#endif
#else
		SCryEngineStatsModuleInfo &moduleInfo = stats.memInfo.modules[i];
		const char *szModule = moduleInfo.name;
#endif
		AddRow();
		nRows++;
		AddCell( szModule,CELL_BOLD );
		AddCell( moduleInfo.usedInModule/1024 );
		AddCell( moduleInfo.memInfo.num_allocations );
		AddCell( moduleInfo.memInfo.allocated/1024 );
#if !defined(PS3)
		AddCell( "" );
		AddCell( moduleInfo.moduleStaticSize/1024 );
		AddCell( (uint32)moduleInfo.SizeOfCode/1024 );
		AddCell( (uint32)moduleInfo.SizeOfInitializedData/1024 );
		AddCell( (uint32)moduleInfo.SizeOfUninitializedData/1024 );
#endif
#if !defined(PS3) || (defined(PS3) && !defined(_LIB)) 
		AddCell( (uint32)(moduleInfo.memInfo.CryString_allocated/1024) ); 
		AddCell( (uint32)(moduleInfo.memInfo.STL_allocated/1024) ); 
		AddCell( (uint32)(moduleInfo.memInfo.STL_wasted/1024) );
		AddCell( (uint32)((moduleInfo.memInfo.allocated - moduleInfo.memInfo.requested)/1024) );
#endif
	}

	AddRow();
	AddCell("");
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
#ifndef PS3
	AddCell("");
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
	AddCell_SumOfRows( nRows );
#endif
	AddRow();
	//AddCell( "" );
	
	//AddRow();
	//AddCell( "Dynamic Textures (KB)",CELL_BOLD );
	//AddCell( m_stats.nAPI_DynTextureSize/(1024) );

	AddRow();
	AddCell( "Lua Memory Usage (KB)",CELL_BOLD );
	AddCell( stats.nSummaryScriptSize/(1024) );
	AddRow();
#if !defined(PS3)
	AddCell( "Total Num Allocs",CELL_BOLD );
	AddCell( stats.memInfo.totalNumAllocsInModules );
	AddRow();
#endif
#if !defined(PS3) || defined(SUPP_MTRACE)
	AddCell( "Total Allocated (KB)",CELL_BOLD );
	AddCell( stats.memInfo.totalUsedInModules/1024 );
	AddRow();
#endif
	AddCell( "Total Code and Static (KB)",CELL_BOLD );
	AddCell( stats.memInfo.totalCodeAndStatic/1024 );
	AddRow();
#if defined(PS3)
	#if defined(_LIB)
		const SCryEngineStatsModuleInfo &modInfo = stats.memInfo.modules[eCryM_Launcher];
		AddCell( "Strings (KB)",CELL_BOLD );
		AddCell( (uint32)(modInfo.memInfo.CryString_allocated/1024) );
		AddRow();
/*
		AddCell( "STL (KB)",CELL_BOLD );
		AddCell( (uint32)(modInfo.memInfo.STL_allocated/1024) );
		AddRow();
		AddCell( "STL Wasted (KB)",CELL_BOLD );
		AddCell( (uint32)(modInfo.memInfo.STL_wasted/1024) );
		AddRow();
*/
	#endif
#else
	AddRow();
#endif
	AddRow();
	AddCell( "API Textures (KB)",CELL_BOLD );
	AddCell( (stats.nSummary_TexturesPoolSize + stats.nSummary_UserTextureSize + stats.nSummary_EngineTextureSize )/(1024*1024) );
	AddRow();
	AddCell( "API Meshes (KB)",CELL_BOLD );
	AddCell( stats.nAPI_MeshSize/1024 );
}

//////////////////////////////////////////////////////////////////////////
void CStatsToExcelExporter::ExportTimeDemoInfo()
{
	if (!GetISystem()->GetITestSystem())
		return;
	STimeDemoInfo* pTD = GetISystem()->GetITestSystem()->GetTimeDemoInfo();
	if (!pTD)
		return;
	
	NewWorksheet( "TimeDemo" );
	FreezeFirstRow();

	XmlNodeRef Column;
	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",400 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );

	AddRow(); AddCell( "Play Time:",CELL_BOLD ); AddCell( pTD->lastPlayedTotalTime );
	AddRow(); AddCell( "Num Frames:",CELL_BOLD ); AddCell( pTD->nFrameCount );
	AddRow(); AddCell( "Average FPS:",CELL_BOLD ); AddCell( pTD->lastAveFrameRate );
	AddRow(); AddCell( "Min FPS:",CELL_BOLD ); AddCell( pTD->minFPS ); AddCell( "At Frame:" ); AddCell( pTD->minFPS_Frame );
	AddRow(); AddCell( "Max FPS:",CELL_BOLD ); AddCell( pTD->maxFPS ); AddCell( "At Frame:" ); AddCell( pTD->maxFPS_Frame );
	AddRow(); AddCell( "Average Tri/Sec:",CELL_BOLD ); AddCell( (uint32)(pTD->nTotalPolysPlayed/pTD->lastPlayedTotalTime) );
	AddRow(); AddCell( "Average Tri/Frame:",CELL_BOLD ); AddCell( (uint32)(pTD->nTotalPolysPlayed/pTD->nFrameCount) );
	AddRow(); AddCell( "Played/Recorded Tris ratio:",CELL_BOLD ); AddCell( pTD->nTotalPolysRecorded ? (float)pTD->nTotalPolysPlayed/pTD->nTotalPolysRecorded : 0.f );

	//////////////////////////////////////////////////////////////////////////
	NewWorksheet( "TimeDemoFrames" );
	FreezeFirstRow();

	Column = m_CurrTable->newChild("Column"); Column->setAttr( "ss:Width",80 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );
	Column = m_CurrTable->newChild("Column");	Column->setAttr( "ss:Width",80 );
	AddRow();
	m_CurrRow->setAttr( "ss:StyleID","s25" );
	AddCell( "Frame Number" );
	AddCell( "Frame Rate" );
	AddCell( "Rendered Polygons" );
	AddCell( "Draw Calls" );

	AddRow();

	for (int i = 0; i < pTD->nFrameCount; i++)
	{
		AddRow();
		AddCell( i );
		AddCell( pTD->pFrames[i].fFrameRate );
		AddCell( pTD->pFrames[i].nPolysRendered );
		AddCell( pTD->pFrames[i].nDrawCalls );
	}
}

void CStatsToExcelExporter::ExportFPSBuckets()
{
	//get perfHUD Export stats
	ICryPerfHUD *perfHUD = GetISystem()->GetPerfHUD();

	if(perfHUD)
	{
		int numBuckets=0;
		float totalTime=0;
		const ICryPerfHUD::FpsBucket* fpsBuckets = perfHUD->GetFpsBuckets(numBuckets, totalTime);

		if(fpsBuckets && totalTime>0.f)
		{
			AddRow();
			AddRow();
			m_CurrRow->setAttr( "ss:StyleID","s25" );

			AddCell( "Frame Rate Bucket" );
			AddCell( "Time Spent%" );

			for(int i=0; i<numBuckets; i++)
			{
				AddRow();
		
				char buf[32];
				sprintf_s(buf, ">=%.1f FPS", fpsBuckets[i].targetFps);
				AddCell( buf );
				
				float percentAtTarget = 100.f * (fpsBuckets[i].timeAtTarget / totalTime);
				AddCell(percentAtTarget);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
static void SaveLevelStats( IConsoleCmdArgs *pArgs )
{
	string levelName = "no_level";

	ICVar *sv_map = gEnv->pConsole->GetCVar("sv_map");
	if (sv_map)
		levelName = sv_map->GetString();

	levelName = PathUtil::GetFileName(levelName);

	bool bDepends = true;
	if (pArgs->GetArgCount() > 1)
		bDepends = true;

	CEngineStats engineStats(bDepends);

	// level.xml
	{
		CStatsToExcelExporter excelExporter;

		string filename = PathUtil::ReplaceExtension(levelName,"xml");

#ifdef XENON
		filename = string(levelName) + "_X360.xml";
#else
  #ifdef PS3
	  	filename = string(levelName) + "_PS3.xml";
  #endif
#endif

		excelExporter.ExportToFile( engineStats.m_stats,filename );

		CryLog("SaveLevelStats exported '%s'",filename.c_str());
	}

	// level_dependencies.xml
	if (bDepends)
	{
		CStatsToExcelExporter excelExporter;

		engineStats.m_ResourceCollector.ComputeDependencyCnt();

		string filename = PathUtil::ReplaceExtension(string("depends_")+levelName,"xml");

		excelExporter.ExportDependenciesToFile( engineStats.m_ResourceCollector,filename );	

		// log to log file - modifies engineStats.m_ResourceCollector data
//		engineStats.m_ResourceCollector.LogData(*gEnv->pLog);
		CryLog("SaveLevelStats exported '%s'",filename.c_str());
	}
	// Clean STL after heavy operation
	STLALLOCATOR_CLEANUP



}

void RegisterEngineStatistics()
{
	REGISTER_COMMAND("SaveLevelStats",SaveLevelStats,0,
		"Calling this command creates multiple XML files with level statistics.\n"
		"The data includes file usage, dependencies, size in more/disk.\n"
		"The files can be loaded in Excel.");
}
