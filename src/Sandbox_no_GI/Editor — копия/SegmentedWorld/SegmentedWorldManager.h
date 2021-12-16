#ifndef __SegmentedWorldManager_h__
#define __SegmentedWorldManager_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "SWFwd.h"
#include "VersionCommon.h"
#include "SWCommon.h"
#include "ISegmentedWorldDoc.h"

class CHyperGraph;
namespace sw{  CSegmentedWorldDoc* GetInternalSWDoc(); }

SW_NAMESPACE_BEGIN();

	// world data console commands
	enum EWDBConsoleComands {
		WDBCC_Lock = 0,
		WDBCC_Save,
		WDBCC_Commit,
		WDBCC_Revert,
		WDBCC_Rollback,
		WDBCC_Unlock,
		WDBCC_COUNT,
	};

	enum ESWMsgTypes {
		SWMsgType_AccessDenied = 0,
		SWMsgType_AccessDenied_Object,
		SWMsgType_AccessDenied_Graph,
		SWMsgType_Disabled,
	};

	enum LevelListTypes {
		LevelList_All = -1,
		LevelList_Local,
		LevelList_SW,
	};

	enum EWDBStates {
		WDBState_Unknown = 0,
		WDBState_Default,
		WDBState_Missing,
		WDBState_LockedByMe,
		WDBState_LockedByOther,
		WDBState_Modified,
		WDBState_OutOfDate,
		WDBState_Conflicted
	};

	enum EOpenResult {
		OpenResult_NoError = 0,
		OpenResult_NotCached = 1,
		OpenResult_OutOfMemory,
		OpenResult_FileOpError,
		OpenResult_AlreadyOpen,
	};

SW_NAMESPACE_END();



struct CDeferredCommandQueue
{
	struct TDeferredCommand
	{
		sw::TWorldName wname;
		CRect rcArea;
		string strCmd;
		CTimeValue timeSleep;
		TDeferredCommand(){}
		bool IsNull()const { return strCmd.empty() && wname.Name.empty(); }
	};
	typedef std::deque<TDeferredCommand> TDeferredCommandQueue;

protected:
	CDeferredCommandQueue()
	:m_nDeferredCCPauseReqCount(0)
	{}
public:
	void Pause(bool bPause) { m_nDeferredCCPauseReqCount += bPause?1:-1; assert(m_nDeferredCCPauseReqCount>=0);}
	void Sleep_(CTimeValue timeIn);
	void ExecOne();
	void AddCommand(TDeferredCommand const& cmd) { m_cmdQueue.push_back(cmd); }

private:
	int m_nDeferredCCPauseReqCount;					// request count for pausing deferred ConsoleCommand
	CTimeValue m_timeDeferredCCSleepUntil;					// insert sleep between executions
	TDeferredCommandQueue m_cmdQueue;

	friend CSegmentedWorldManager;
};


struct CSegmentedWorldManager : public IEditorNotifyListener
{
	static CSegmentedWorldManager &Get();


	CSegmentedWorldManager();
	void Init();

	ISegmentedWorldDoc &OpenDoc(const char *szPath, bool bForce);
	ISegmentedWorldDoc *CreateNewDoc(const char *szPath);
	bool SaveAsSWLevel( CCryEditDoc* pDoc, const char* szFullName );
	bool ConvertToSW( LPCTSTR filename );

	bool MergeLevel(CString const &strDestFile, CString const& strSrcFile, Vec2i const& vDestTopLeft, Vec2i const& vSrcTopLeft, Vec2i const& vSize );
	bool RemoveSegments(CRect const& rcArea, const char* szWorldName = NULL);
	bool RemoveSegments(std::vector<CPoint> const& wcs, const char* szWorldName = NULL);
	bool IsAutomate();
	bool IsOfflineMode();
	int GetMode()									{ return m_nSWMode; }
	bool IsMapConverting(); // TODO: refactor this function  -RL

	bool IsSWDoc()const  { return m_pSWDoc != NULL; }
	ISegmentedWorldDoc& GetDoc()const { return m_pSWDoc? *(ISegmentedWorldDoc*)m_pSWDoc : ISegmentedWorldDoc::GetNullObject(); }
	void DeleteDoc();
	static bool IsActive();
	bool IsIgnoringChanges();
	void IgnoreChanges(bool bIgnore);
	bool IsHot() const { return false/*(m_vt == sw::VT_HOT)*/; }
	bool NeedHotHistoryBeDeleted() const { return IsHot() && m_bDeleteHotHistoryRecord; }

	static TSWDataStructureVersion MayUpgradeLevel( sw::ISWDataPersistence* pPersist );
	static TSWDataStructureVersion GetSWDataStructureVersion();
	static bool IsLevelInMultiPack();
	static bool IsSWLevel( const char * szPath );
	static void GetPakPath(CString &strSegmentPakFileName, CString &strSegmentPakPath, CString &strSegmentFileBlockPath, int wx, int wy, bool bMultiPak, bool bCreateDir = true);
	static void GetSinglePakPath(CString &strSegmentPakFileName, CString &strSegmentPakPath);
	static void GetMultiPakPath(CString &strSegmentPakFileName, CString &strSegmentPakPath, int wx, int wy, bool bCreateDir);

	//static void GetDBCredetentials(int &DBType, char *chDBServer, char *chDBUser, char *chDBPassword, char *chDBName);

	// support for open map UI
	static void FindLocalWorlds(std::vector<sw::TWorldName> &lstNames);
	static void ListWorlds(std::vector<sw::TWorldName> &lstNames, int nLevelListType = sw::LevelList_All);
	void OnOpenWorld();
	sw::EOpenResult OpenWorld(const sw::TWorldName &wnName, bool bReopenOnly = false, const CRect* rcWorldRect = NULL, int nSwZoomLvl = 0, const Vec2 &ptOfs = Vec2(0,0));
	void GetLastOpenedWorld(string &strName, int nLevelListType);
	//static const char* MakeDBName(char *pBuf, const char *pcName);
	static const char* GetVersionTypeName(sw::EVersionType vt);
	static const char* GetSDBlockName(sw::ESDBType sdbt);
	static const char* GetWDBlockName(sw::EWDBType wdbt);
	int GetDebugDraw() { return m_nDrawBoxes; }
	void SetDebugDraw(int val) { m_nDrawBoxes = val; }


	//static void InitArrXmlAr(std::vector<CXmlArchive*> &arrXmlAr, CXmlArchive &xmlAr);

	//GD lock check (GD = Global Data -> data which is not segment based)
	static bool IsVegetationMapLockedByMe();

	static bool CanModifyWorldData(sw::EWDBType eType, bool bLock);
	
	static void InitSegmentedWorldInfo();

	// Heightmap utils
	static void CoordToAABB(AABB &destBox, int x1, int y1, int x2, int y2, bool bIsInWorldCoord, bool bWithAdditionalSafeZone = true );
	static UINT GetSegmentSizeInUnits();
	static int GetSegmentSizeMeters();

	// handle selection change of RollUpBar
	void OnRollUpBarSelect(int nRollUpCtrlID);

	//bool NeedReloadLevel();
	//void OnWorldDataApplied();

	// persistence & version control
	static sw::TPersistencePtr CreateTempPersistence(const char* pszLevelPath, bool bUseNullVC = false);

	//Message box
	static void SWMsgBox(sw::ESWMsgTypes nMsgType, CBaseObject* pOptional = NULL);
	static void SWMsgBox(const char* format,...);
	static int SWMsgBoxQuestion( const char* format,... );

	// event handlers from IEditorNotifyListener
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	// callback for doc_segmented_world_mode
	static void OnModeChange(ICVar* pVar);
	void DoOnModeChange(ICVar* pVar);
	static void OnSWDrawBoundingBoxVarChange(ICVar *pArgs);

	// console command handling
	static void ConsoleCommandEntry(IConsoleCmdArgs *pArgs);
	CDeferredCommandQueue& DeferredCommandQueue() {return m_defCmdQueue;}
	const CDeferredCommandQueue& DeferredCommandQueue()const {return m_defCmdQueue;}

	static void GetMissingSegmentsInRect(const char *levelpath, const CRect &rect, std::list<CRect> &segRect);
	static bool GetWorldBounds(const char *levelpath, CRect &rect);

protected:
	CSegmentedWorldDoc* GetDocI()const { return m_pSWDocInternal; };	// the internal interface

	bool ConsoleCommandHandler(IConsoleCmdArgs *pArgs);
	void RegisterConsoleVariables();

private:
	CSegmentedWorldDoc* m_pSWDocInternal;		// this is a global doc pointer for all internal data structures to use
	CSegmentedWorldDoc* m_pSWDoc;				// this is non-NULL only if the doc is fully initialized
	CSingleDocTemplate* m_pDocTemplate;			// the sw doc template
	CDeferredCommandQueue m_defCmdQueue;

	ICVar* doc_enable_segmented_world;
	ICVar* doc_segmented_world_mode;

	int m_nSWMode;
	int m_nDrawBoxes; // segment bounds drawing mode
	int s_nIgnoringChanges;
	bool m_bDeleteHotHistoryRecord;

	static CSegmentedWorldManager s_instance;		// the singleton

	friend sw::CSWModeFakeAutoGuard;
	friend CSegmentedWorldDoc* sw::GetInternalSWDoc();
	friend CDeferredCommandQueue;
};


SW_NAMESPACE_BEGIN();

// accessor for internal use of the SWDoc
inline CSegmentedWorldDoc* GetInternalSWDoc(){return CSWManager::Get().GetDocI();}

class CSWModeFakeAutoGuard
{
	int m_nTmpSWMode;
public:
	CSWModeFakeAutoGuard(int nFakeMode) { m_nTmpSWMode = CSWManager::Get().m_nSWMode; CSWManager::Get().m_nSWMode = nFakeMode; }
	~CSWModeFakeAutoGuard() { CSWManager::Get().m_nSWMode = m_nTmpSWMode; }
};

class CIgnoreChangesAutoGuard
{
	bool bReleased;
public:
	CIgnoreChangesAutoGuard():bReleased(false) { CSWManager::Get().IgnoreChanges(true); }
	~CIgnoreChangesAutoGuard() { Release(); }

	void Release() { if (!bReleased) { CSWManager::Get().IgnoreChanges(false); bReleased = true;} }
};
SW_NAMESPACE_END();

#define SW_NAMED_IGNORE_CHANGES_AUTOGUARD(name) ::sw::CIgnoreChangesAutoGuard name;		// keep this as a macro for easier changing hundreds of injecting code into other systems.
#define _SW_IGNORE_CHANGES_AUTOGUARD_2(nameprefix,salt) SW_NAMED_IGNORE_CHANGES_AUTOGUARD( nameprefix##salt );
#define _SW_IGNORE_CHANGES_AUTOGUARD_1(nameprefix,salt) _SW_IGNORE_CHANGES_AUTOGUARD_2( nameprefix, salt );
#define SW_IGNORE_CHANGES_AUTOGUARD		_SW_IGNORE_CHANGES_AUTOGUARD_1(__sw_ignore_changes_autoguard_, __LINE__)

#include "SWNotify.h"

#endif // __SegmentedWorldManager_h__
