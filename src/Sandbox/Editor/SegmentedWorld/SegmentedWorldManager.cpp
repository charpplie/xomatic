#include "StdAfx.h"
#include "InternalCommon.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorldDoc.h"
#include "SCDataPersistence.h"
#include "NullVersionControl.h"
#include "NativeSCMAdapter.h"
#include "DataUpgrade.h"

// UIs
#include "SegmentedWorldMiniMapUpdater.h"
#include "SegmentedWorldNewWorldDlg.h"
#include "SegmentLevelMerger.h"
#include "SegmentsRemover.h"

#include <ISystem.h>
#include "Terrain/TerrainManager.h"
#include "Objects/EntityObject.h"
#include "./HyperGraph/HyperGraph.h"
#include "./HyperGraph/FlowGraph.h"
#include "CryEditDoc.h"
#include "ViewPane.h"
#include "Util/PakFile.h"
#include <EngineSettingsManager.h>
#include "ISourceControl.h"
#include "CryEdit.h"
#include "GameEngine.h"

using namespace sw;

class CSWSingleDocTemplate : public CCrySingleDocTemplate
{
public:
	CSWSingleDocTemplate(UINT nIDResource, 
						 CRuntimeClass* pDocClass,
						 CRuntimeClass* pFrameClass, 
						 CRuntimeClass* pViewClass, 
						 const char* szOverrideDocStrings = NULL)
		:CCrySingleDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass)
	{
		if (szOverrideDocStrings)
			m_strDocStrings = szOverrideDocStrings;
	}
};


//-----------------------------------------------------------------------------------------------
void CSegmentedWorldManager::ConsoleCommandEntry(IConsoleCmdArgs *pArgs)
{
	// non doc commands  (SW module commands)
	bool bHandled = GetIEditor()->GetSegmentedWorldManager()->ConsoleCommandHandler(pArgs);
	if (bHandled)
		return;

	auto pswdoc = GetIEditor()->GetSegmentedWorldManager()->GetDocI();
	if (pswdoc)
	 pswdoc->ConsoleCommand(pArgs);
}


CSegmentedWorldManager::CSegmentedWorldManager()
	:m_pSWDoc(NULL)
	,m_pSWDocInternal(NULL)
	,m_pDocTemplate(NULL)
	,m_pSWDebugDraw(NULL)
	,m_pForceGlobalInSWForNewAI(NULL)
{
	m_nOnlineMode = 1;
	m_nDrawBoxes = 1;
	s_nIgnoringChanges = 0;
}

CSegmentedWorldManager::~CSegmentedWorldManager()
{
	GetIEditor()->UnregisterNotifyListener(this);
}

//! Initializer for the manager
void CSegmentedWorldManager::Init()
{
	// SW Doc template
	CString strSWDocStrings;
	strSWDocStrings.LoadStringA(IDR_MAINFRAME_SW);
	CSingleDocTemplate* pDocTemplate = new CSWSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CSWDocAdapter),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CLayoutViewPane),
		strSWDocStrings);
	AfxGetApp()->AddDocTemplate(pDocTemplate);
	m_pDocTemplate = pDocTemplate;

	// register console commands and cvars
	REGISTER_COMMAND( "sw", &CSegmentedWorldManager::ConsoleCommandEntry, VF_NULL, "Control segmented world, use 'sw help' for more info" );
	GetISystem()->GetIConsole()->GetCVar("sw_draw_bbox")->SetOnChangeCallback(OnSWDrawBoundingBoxVarChange);
	m_pForceGlobalInSWForNewAI = REGISTER_INT("sw_forceGlobalInSWForNewAI", 0, 0, "When new AI entity is added to the level, set its 'Global In SegmentedWorld' param to true.");
	GetIEditor()->RegisterNotifyListener(this);
}

void CSegmentedWorldManager::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnIdleUpdate:
		if (m_pSWDoc)
			m_pSWDoc->UpdateOnIdle();

		if(m_pSWDebugDraw)
			m_pSWDebugDraw->UpdateOnIdle();

		m_defCmdQueue.ExecOne();
		break;
	}
}

extern CCryEditDoc* theDocument;

bool CSegmentedWorldManager::ConvertToSW( LPCTSTR  )
{
	AUTO_LOCK_BIG(g_csBigLock);
	if (m_pSWDoc)
	{
		AfxMessageBox( _T("The World is already Segmented"), MB_ICONEXCLAMATION|MB_OK );
		return false;
	}
	
	CSWNewWorldDlg dlg;
	if (IDOK != dlg.DoModal())
		return false;

	string worldname;
	dlg.GetSelectedWorld(worldname);

	if (worldname.empty())
	{
		AfxMessageBox( _T("Must Write name for World"), MB_ICONEXCLAMATION|MB_OK );
		return false;
	}
	
	string fullPathName = gEnv->pCryPak->GetGameFolder() + string("/levels/") + worldname.c_str();
	if (!CFileUtil::PathExists(fullPathName.c_str()))
		CFileUtil::CreateDirectory(fullPathName.c_str());

	string fullName;
	fullName.Format("%s/%s.scry", fullPathName.c_str(), worldname.c_str());

	if(PathFileExists(fullName.c_str()))
	{
		AfxMessageBox( _T("Level with this name already exists, choose another name."), MB_ICONEXCLAMATION|MB_OK );
		return false;
	}

	CSegmentedWorldDoc::s_bAutomate = true;
	CSegmentedWorldDoc::s_bMapConverting = true;
	bool bRes = false;
	
	CCryEditDoc *doc = 0;
	doc = (CCryEditDoc*)GetIEditor()->GetDocument();
	ASSERT_KINDOF(CCryEditDoc,doc);

	Recti rcWorld;
	{
		CHeightmap* pHMap = GetIEditor()->GetHeightmap();
		int w = pHMap->GetHeight() / SEGMENT_SIZE_UNITS;
		int h = pHMap->GetWidth() / SEGMENT_SIZE_UNITS;

		rcWorld = Recti(0, 0, w, h);

		CRGBLayer *TerrainRGBLayer = pHMap->GetRGBLayer();
		if (w != TerrainRGBLayer->GetTileCountX() || h != TerrainRGBLayer->GetTileCountY())
			TerrainRGBLayer->Resize(w, h, SEGMENT_SIZE_UNITS);
	}


	// --== STEP-1: Generate and fill all SW structures.	==--
	//bRes = doc->OnSaveDocument(fullName.c_str());
	if (!SaveAsSWLevel(doc, fullName, rcWorld))
		return false;


	// --== STEP-2: Open it as SW level						==--
	//GetDocI()->worlddata_Unapply(); // because we will have to apply it again
	string sName;
	sName.Format("%s",GetDocI()->GetWorldName());
	//GetIEditor()->GetDocument()->OnCloseDocument();
	OpenWorld(sName, false, &rcWorld);

	// --== STEP-3: Re-generate surface texture				==--
	GetIEditor()->GetDocument()->SetDocumentReady(true);
	((CCryEditApp*)AfxGetApp())->GenerateTerrainTexture();


	// --== STEP-4: Save all as SW level					==--
	GetISystem()->GetIConsole()->ExecuteString("sw gd lock");
	doc = GetIEditor()->GetDocument();
	if(doc->DoSave(fullName, TRUE))
		bRes = bRes && true;
	
	// --== STEP-5: Reopen as SW level???					==--
	OpenWorld(sName, false, &rcWorld);

	CSegmentedWorldDoc::s_bAutomate = false;

	// TODO: implement 'commit'
	//GetISystem()->GetIConsole()->ExecuteString("sw commit");

	//GetDocI()->GenerateSegmentedMap();
	SetCurrentDirectoryW( GetIEditor()->GetMasterCDFolder() );
	GetIEditor()->GetDocument()->SetDocumentReady(true);

	CSegmentedWorldDoc::s_bMapConverting = false;
	return bRes;
}
bool CSegmentedWorldManager::IsAutomate()
{
	return CSegmentedWorldDoc::s_bAutomate;
}

void CSegmentedWorldManager::DeleteDoc()
{
	SAFE_DELETE(m_pSWDoc);
	SAFE_DELETE(m_pSWDebugDraw);
	m_pSWDocInternal = NULL;

	// disable all the sw flags
	//CSegmentedWorldDoc::s_bAutomate = false;
	//CSegmentedWorldDoc::s_bMapConverting = false;
}

bool CSegmentedWorldManager::IsActive()
{
	return GetIEditor()->Get3DEngine()->IsSegmentOperationInProgress();
}

bool CSegmentedWorldManager::IsIgnoringChanges()
{
	return (s_nIgnoringChanges > 0);
}

void CSegmentedWorldManager::IgnoreChanges(bool bIgnore)
{
	if (bIgnore)
		s_nIgnoringChanges++;
	else
		s_nIgnoringChanges--;
}

bool CSegmentedWorldManager::IsOfflineMode()
{
	return m_nOnlineMode == 0;
}

bool CSegmentedWorldManager::IsMapConverting()
{
	return CSegmentedWorldDoc::s_bMapConverting;
}

void CSegmentedWorldManager::OnModeChange(ICVar* pVar)
{
	GetIEditor()->GetSegmentedWorldManager()->DoOnModeChange(pVar);
}

void CSegmentedWorldManager::DoOnModeChange(ICVar* pVar)
{
	AUTO_LOCK_BIG(g_csBigLock);
	CSegmentedWorldDoc* psw;
	psw = GetDocI();
	static bool s_bQuiet = false;

	if (s_bQuiet)
	{
		m_nOnlineMode = pVar->GetIVal();
		//if (CGridMapDlg::Get())
		//	CGridMapDlg::Get()->OnSWModeChange(m_nOnlineMode);
		return;
	}

	if (psw && !s_bQuiet)
	{
		if (IsOfflineMode() && pVar->GetIVal() != 0) //if going from offline (0) to online (!= 0) mode
		{	
			if (psw && psw->IsSWModified())
			{
				const char *pcMsg;
				pcMsg = "Going to ONLINE mode! Do you want TRY KEEP your changes?";
				int nMsgRes = MessageBox(0, pcMsg, "Try Save?", MB_YESNOCANCEL);
				if (IDYES == nMsgRes)
				{
					std::vector<TWorldData*> wdJustLocked;
					std::vector<TSegmentData*> sdJustLocked; 
					std::vector<TLayerData*> ldJustLocked;

					bool bRes = psw->TryLockModified(wdJustLocked, sdJustLocked, ldJustLocked);
					if (bRes)
					{
						//CSWModeFakeChange swFakeMode(1);
						//bool bSaveRes = psw->Save(true);
						//psw->SWModified(!bRes);
					}
					else
					{
						const char *pcMsgResolve = "CONFLICTS FOUND!!! DO YOU WANT TO RESOLVE?\n\r"
																			 "\tYES - will revert conflicts and keep other modifications\n\r"
																			 "\tNO - will revert ALL YOUR modifications\n\r"
																			 "\tCANCEL - will return YOU to OFFLINE mode";
						int nMsgResolveRes = MessageBox(0, pcMsgResolve, "Try RESOLVE?", MB_YESNOCANCEL);
						if (IDYES == nMsgResolveRes)
						{
							//bRes = psw->TryResolveSave(false, true);
							////psw->SWModified(!bRes);
						}
						else if (IDNO == nMsgResolveRes)
						{
							bool bUnlockJustLocked = psw->TryUnLockJustLocked(wdJustLocked, sdJustLocked, ldJustLocked);
							psw->RevertAll();
							psw->OnSWModified(false);
						}
						else if (IDCANCEL == nMsgResolveRes )
						{
							bool bUnlockJustLocked = psw->TryUnLockJustLocked(wdJustLocked, sdJustLocked, ldJustLocked);
							s_bQuiet = true;
							pVar->Set(m_nOnlineMode);
							s_bQuiet = false;
							return;
						}
					}
				}
				else if (IDNO == nMsgRes)
				{
					psw->RevertAll();
					psw->OnSWModified(false);
				}
				else if (IDCANCEL == nMsgRes)
				{
					s_bQuiet = true;
					pVar->Set(m_nOnlineMode);
					s_bQuiet = false;
					return;
				}
			}
		}
		else if (m_nOnlineMode != 0 && pVar->GetIVal() == 0)
		{
			if (psw && psw->IsSWModified())
			{
				const char *pcMsg;
				pcMsg = "Going to OFFLINE mode! Do you want save your changes?";

				int nMsgRes = MessageBox(0, pcMsg, "Save?", MB_YESNOCANCEL);
				if (IDYES == nMsgRes)
				{
					bool bRes = psw->Save(true);
					psw->OnSWModified(!bRes);
				}
				else if (IDNO == nMsgRes)
				{
					psw->RevertAll();
					psw->OnSWModified(false);
				}
				else if (IDCANCEL == nMsgRes)
				{
					s_bQuiet = true;
					pVar->Set(m_nOnlineMode);
					s_bQuiet = false;
					return;
				}
			}
		}
	}
	m_nOnlineMode = pVar->GetIVal();
	//if (CGridMapDlg::Get())
	//	CGridMapDlg::Get()->OnSWModeChange(m_nOnlineMode);
	SWLog("SWMode changed\n"); 
}

// CS - 
void CSegmentedWorldManager::GetPakPath(CString &strSegmentPakFileName, CString &strSegmentPakPath, CString &strSegmentFileBlockPath, int wx, int wy, bool bMultiPak, bool bCreateDir)
{
	GetIEditor()->GetSegmentedWorldManager()->GetDocI()->GetPersistence()->GetPakPath(strSegmentPakFileName, strSegmentPakPath, strSegmentFileBlockPath, TWorldCoords(wx, wy), bMultiPak, bCreateDir);
}

bool CSegmentedWorldManager::IsLevelInMultiPack()
{
	return s_bLevelMultiPack;
}

ISegmentedWorldDoc* CSegmentedWorldManager::CreateNewDoc(const char *szPath, const Recti& rcWorld)
{
	assert(szPath);
	if (!szPath)
		return NULL;
	
	GetIEditor()->Notify( eNotify_OnBeginSWNewScene );

	int dummyForScopedExit = 0;
	auto scopeExitCode = [](int*){
		GetIEditor()->Notify( eNotify_OnEndSWNewScene );
	};
	std::unique_ptr<int, decltype(scopeExitCode)> scopeExitGuard(&dummyForScopedExit, scopeExitCode);

	if (m_pSWDoc)
	{
		assert(0);
		DeleteDoc();
	}
	m_pSWDocInternal = new CSegmentedWorldDoc();
	m_pSWDocInternal->RcWorldBounds() = Recti(rcWorld.Min.x, rcWorld.Min.y, rcWorld.Max.x - 1, rcWorld.Max.y - 1);

	// re-create VC & persistence if offline mode changed
	TPersistencePtr pPersist = CreateTempPersistence( Path::GetRelativePath(Path::GetPath(szPath)));

	if (!m_pSWDocInternal->Init(szPath, pPersist, &rcWorld) || !m_pSWDocInternal->SaveLevel(szPath))
	{
		assert(0);
		SAFE_DELETE(m_pSWDocInternal);
		return NULL;
	}

	// pass it to public accessible pointer only when it's initialized
	m_pSWDoc = m_pSWDocInternal;
	m_pSWDebugDraw = new CSegmentedWorldDebugDraw;

	return m_pSWDoc;
}

ISegmentedWorldDoc &CSegmentedWorldManager::OpenDoc(const char *szPath, bool bForce)
{
	assert(szPath);
	if (!szPath || !IsSWLevel(szPath))
	{
		if (m_pSWDoc)
			DeleteDoc();
		return ISegmentedWorldDoc::GetNullObject();
	}

	GetIEditor()->Notify( eNotify_OnBeginSWNewScene );

	int dummyForScopedExit = 0;
	auto scopeExitCode = [](int*){
		GetIEditor()->Notify( eNotify_OnEndSWNewScene );
	};
	std::unique_ptr<int, decltype(scopeExitCode)> scopeExitGuard(&dummyForScopedExit, scopeExitCode);

	if (m_pSWDoc)
	{
		//// this is a reload
		//if (0 == CSWDoc::NormalizeLevelPath(szPath).CompareNoCase(m_pSWDocInternal->GetLevelPath()))
		//{
		//	m_pSWDoc = NULL;
		//	m_pSWDocInternal->CleanUp(true);
		//	m_pSWDocInternal->Init(true,szPath, bForce, m_pSWDocInternal->GetPersistence());
		//	m_pSWDoc = m_pSWDocInternal;

		//	if (CGridMapDlg::Get())
		//		CGridMapDlg::Get()->OnLevelChanged();

		//	return *m_pSWDoc;
		//}

		DeleteDoc();
	}
	m_pSWDocInternal = new CSegmentedWorldDoc();

	// TODO: re-create VC only if level or offline mode changed
	CString levelPath = Path::GetRelativePath(Path::GetPath(szPath));
	TPersistencePtr pPersistTmp = CreateTempPersistence(levelPath);

	GetIEditor()->GetSystem()->GetIPak()->OpenPack(m_scryPath);

	// upgrade data if needed
	if (MayUpgradeLevel(levelPath) != GetSWDataStructureVersion()
		|| !m_pSWDocInternal->Init(szPath, pPersistTmp, NULL) || !m_pSWDocInternal->LoadLevel(szPath))
	{
		assert(0);
		SAFE_DELETE(m_pSWDocInternal);
		return ISegmentedWorldDoc::GetNullObject();
	}


	// publish it after initialized
	m_pSWDoc = m_pSWDocInternal;
	m_pSWDebugDraw = new CSegmentedWorldDebugDraw;

	return *m_pSWDoc;
}

//rollUpBar notification
void CSegmentedWorldManager::OnRollUpBarSelect(int nRollUpCtrlID)
{
	CSegmentedWorldDoc::s_nRollUpCtrlID = nRollUpCtrlID;
	CSWMiniMapUpdater::s_nRollUpCtrlID = nRollUpCtrlID;
}
//Messages
void CSegmentedWorldManager::SWMsgBox(ESWMsgTypes nMsgType, CBaseObject* pOptional)
{
	if (GetIEditor()->GetSegmentedWorldManager()->GetDocI() != NULL)
	{
		CString msg;
		switch (nMsgType)
		{
		case SWMsgType_AccessDenied:
			msg.Format("Access denied!!!");
			break;
		case SWMsgType_AccessDenied_Object:
			{
				if (pOptional)
				{
					msg.Format("Access denied!!!\r\nObject: %s\r\nis locked by other user or is out of loaded part of world", pOptional->GetName());
				}
				else
					msg.Format("Access denied!!!\r\n\r\nObject is locked by other user or is out of loaded part of world");
			}
			break;
		case SWMsgType_AccessDenied_Graph:
			{
				if (pOptional)
				{
					msg.Format("Access denied!!!\r\n\r\nGraph owner (%s) is locked by other user or is out of loaded part of world", pOptional->GetName());
				}
				else
					msg.Format("Access denied!!!\r\n\r\nGraph owner is locked by other user or is out of loaded part of world");
			}
			break;
		case SWMsgType_Disabled:
			msg.Format("Action is disabled in Segmented World");
			break;
		default:
			return;
		}
		AfxMessageBox( msg, MB_ICONEXCLAMATION|MB_OK );
	}
}

int CSegmentedWorldManager::SWMsgBoxQuestion( const char* format,... )
{
	CString msg;
	va_list args;
	va_start(args,format);
	msg.FormatV(format,args);
	va_end(args);
	switch (int retval = AfxMessageBox(msg, MB_ICONQUESTION | MB_YESNOCANCEL))
	{
	case IDYES:
	case IDNO:
	case IDCANCEL:
		return retval;
	default:
		assert(0);
		return IDCANCEL;
	}
}

void CSegmentedWorldManager::SWMsgBox( const char* format,... )
{
	CString msg;
	va_list args;
	va_start(args,format);
	msg.FormatV(format,args);
	va_end(args);
	AfxMessageBox(msg, MB_ICONWARNING );
}

bool ISegmentedWorldDoc::LockGraph(CHyperGraph* pGraph, bool bModify)
{
	assert(pGraph);

	if (pGraph && pGraph->IsFlowGraph())
	{
		CFlowGraph *pFlowGraph = static_cast<CFlowGraph*>(pGraph);
		CBaseObject* pObject = pFlowGraph->GetEntity();
		if (! LockObject(pObject, bModify))
		{
			CSegmentedWorldManager::SWMsgBox(SWMsgType_AccessDenied_Graph, pObject);
			return false;
		}
	}

	return true;
}

bool ISegmentedWorldDoc::IsLockedByMe(CHyperGraph* pGraph)
{
	assert(pGraph);

	if (pGraph && pGraph->IsFlowGraph())
	{
		CFlowGraph *pFlowGraph = static_cast<CFlowGraph*>(pGraph);
		CBaseObject* pObject = pFlowGraph->GetEntity();
		if (!IsLockedByMe(pObject))
			return false;

	}

	return true;
}

bool ISegmentedWorldDoc::CanModify(CHyperGraph* pGraph, bool bLock)
{
	assert(pGraph);

	if (pGraph && pGraph->IsFlowGraph())
	{
		CFlowGraph *pFlowGraph = static_cast<CFlowGraph*>(pGraph);
		CBaseObject* pObject = pFlowGraph->GetEntity();
		if(pObject && !CanModify(pObject, false, bLock))
		{
			if (bLock)
				CSegmentedWorldManager::SWMsgBox(SWMsgType_AccessDenied_Graph, pObject);

			return false;
		}
	}
	return true;
}

// heightmap check
///////////////////////////////////////////////////////////////////////////////////
void CSegmentedWorldManager::CoordToAABB(AABB &destBox, int x1, int y1, int x2, int y2, bool bIsInWorldCoord, bool bWithAdditionalSafeZone )
{
	destBox.Reset();
#ifndef SEG_WORLD
	return;
#endif
	if (x1==0 && y1==0 && x2==0 && y2==0)
		destBox.Reset();
	else
	{
		// Here we are making sure that we will update the whole sectors where the heightmap was changed.
		int nTerrainSectorSize = gEnv->p3DEngine->GetTerrainSectorSize();
		if (nTerrainSectorSize == 0)
			return;
		int unitSize = GetIEditor()->GetHeightmap()->GetUnitSize();
		x1*=unitSize;
		y1*=unitSize;

		x2*=unitSize;
		y2*=unitSize;

		if(bIsInWorldCoord)
		{
			x1/=nTerrainSectorSize;
			y1/=nTerrainSectorSize;
			x2/=nTerrainSectorSize;
			y2/=nTerrainSectorSize;
		}

		int nSafeZoneSize = (bWithAdditionalSafeZone ? 1 : 0);
		// Y and X switched by historical reasons.
		if(bIsInWorldCoord)
		{
			destBox.Add(Vec3((y1-nSafeZoneSize)*nTerrainSectorSize,(x1-nSafeZoneSize)*nTerrainSectorSize,-32000.0f));
			destBox.Add(Vec3((y2+nSafeZoneSize)*nTerrainSectorSize,(x2+nSafeZoneSize)*nTerrainSectorSize,+32000.0f));
		}
		else
		{
			destBox.Add(Vec3((x1-nSafeZoneSize)*nTerrainSectorSize,(y1-nSafeZoneSize)*nTerrainSectorSize,-32000.0f));
			destBox.Add(Vec3((x2+nSafeZoneSize)*nTerrainSectorSize,(y2+nSafeZoneSize)*nTerrainSectorSize,+32000.0f));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
sw::EOpenResult CSegmentedWorldManager::OpenWorld(const string& sName, 
												  bool bReopenOnly /*= false*/, 
												  const Recti* rcWorldRect /*= NULL*/,
												  int nSwZoomLvl /*= 0*/, 
												  const Vec2 &ptOfs /*= Vec2(0,0)*/ )
{
	auto levelPath = GetIEditor()->GetGameEngine()->GetLevelPath();
	CDocument *doc = GetIEditor()->GetDocument();
	if (doc)
	{
		string strCurrDocName = PathUtil::ToUnixPath((const char*)doc->GetPathName());
		string strNewDocName = string(levelPath.GetBuffer()) + "/" +sName + ".scry";

		//if is the same document;
		if (strCurrDocName.compare(strNewDocName) == 0 &&
			(GetDocI() != 0))
		{
			return OpenResult_AlreadyOpen;
		}
	}

	auto configPath = levelPath + "/" + "level.cfg";
	if(GetISystem()->GetIPak()->IsFileExist( configPath ))
		GetISystem()->LoadConfiguration( configPath );

	if (!CFileUtil::PathExists(levelPath))
	{
		if (bReopenOnly)
		{
			return OpenResult_NotCached;
		}
		CFileUtil::CreateDirectory(levelPath);
	}

	CString sPath = levelPath + "/" + sName.c_str() + ".scry";
	SaveLastEditInfoForLevel(sPath, rcWorldRect, nSwZoomLvl, ptOfs);

	GetIEditor()->GetDocument()->SetDocumentReady(false);
	AfxGetApp()->OpenDocumentFile(sPath);

	return OpenResult_NoError;
}

const char* CSegmentedWorldManager::GetVersionTypeName( sw::EVersionType vt )
{
	return g_pcVersionTypes[vt];
}


namespace{
int _ParseInt(size_t idx, IConsoleCmdArgs *pArgs, size_t& bOutFailed)
{
	int retval = -1;

	if (bOutFailed)		// skip the rest if previous arg already failed
		return retval;

	if (0 == sscanf(pArgs->GetArg(idx), "%d", &retval))
		bOutFailed = idx;

	return retval;
}
}

//!
// return true if the command is handled
bool CSegmentedWorldManager::ConsoleCommandHandler( IConsoleCmdArgs *pArgs )
{
	int nArg = pArgs->GetArgCount();
	if(nArg < 2)
		return false;

	const char *pcmd = pArgs->GetArg(1);
	const char *pcCmds[1024];
	const char *pcHelps[1024];
	int nCmds = 0;
	bool bCmdHandled = false;

	bool& bRet = bCmdHandled;

	enum{_BASE_OFFSET = 2};

#define CMD(cmd, help) \
	pcCmds[nCmds] = cmd; \
	pcHelps[nCmds] = help; \
	++nCmds; \
	if (!strcmpi(pcmd, cmd) && (bCmdHandled = true))

	CMD("batch", "<level> <cmd> <...> - run <cmd> on every segments of <level>")
	{
		if (pArgs->GetArgCount() < 4)
			return bRet;

		const char *filename = pArgs->GetArg(2);
		if (!IsSWLevel(filename))
			return bRet;

		// make sub command line back again
		// it sounds silly, but this is not performance critical
		string strCmd;
		for (int ii = 3; ii < pArgs->GetArgCount(); ++ii)
		{
			strCmd += pArgs->GetArg(ii);
			strCmd += ' ';
		}
		strCmd.TrimRight(" ");

		Recti rcWorld = m_pSWDoc->RcWorldBounds();
		rcWorld.Max.x += 1;
		rcWorld.Max.y += 1;

		const int nStepSize = 8;		// tweak this
		// remain should take all main part if there are no gain on the other axis
		const int nXRemain = rcWorld.GetWidth() % nStepSize;
		const int nYRemain = rcWorld.GetHeight() % nStepSize;
		const int nXMainPartEnd = rcWorld.Max.x - nXRemain;
		const int nYMainPartEnd = rcWorld.Max.y - nYRemain;

		// command prototype
		CDeferredCommandQueue::TDeferredCommand defcmd;
		defcmd.timeSleep = 2.f;								// the interval between two commands
		defcmd.sName = PathUtil::GetFileName(filename);
		defcmd.strCmd = strCmd;

		// big slices
		for (int y = rcWorld.Min.y; y < nYMainPartEnd; y += nStepSize )
		for (int x = rcWorld.Min.x; x < nXMainPartEnd; x += nStepSize )
		{
			defcmd.rcArea = Recti(x, y, x + nStepSize, y + nStepSize);
			m_defCmdQueue.AddCommand(defcmd);
		}

		// slices on edges
		int nMinStep;
		// edge slices parallel to x-axis AND THE CORNER
		if (nMinStep = RightMostBit(nYRemain))
		{
			for (int y = nYMainPartEnd; y < rcWorld.Max.y; y += nMinStep)
				for (int x = rcWorld.Min.x; x < nXMainPartEnd; x += nMinStep)
				{
					defcmd.rcArea = Recti(x, y, x + nMinStep, y + nMinStep);
					m_defCmdQueue.AddCommand(defcmd);
				}
		}

		// edge slices parallel to y-axis WITHOUT CORNER
		if (nMinStep = RightMostBit(nXRemain))
		{
			for (int x = nXMainPartEnd; x < rcWorld.Max.x; x += nMinStep)
				for (int y = rcWorld.Min.y; y < nYMainPartEnd; y += nMinStep)
				{
					defcmd.rcArea = Recti(x, y, x + nMinStep, y + nMinStep);
					m_defCmdQueue.AddCommand(defcmd);
				}
		}

		// the corner
		if (nMinStep = std::min(RightMostBit(nXRemain),RightMostBit(nYRemain)))
		{
			for (int y = nYMainPartEnd; y < rcWorld.Max.y; y += nMinStep)
				for (int x = nXMainPartEnd; x < rcWorld.Max.x; x += nMinStep)
				{
					defcmd.rcArea = Recti(x, y, x + nMinStep, y + nMinStep);
					m_defCmdQueue.AddCommand(defcmd);
				}
		}

		return bRet;
	}
	CMD("merge", "<width> <height> [dest level] <dest x> <dest y> <src level> [src x] [src y] - merge <src level> into <dest level>")
	{
		enum{ ARG_WIDTH = 0, ARG_HEIGHT, ARG_DESTLEVEL, ARG_DESTX, ARG_DESTY, ARG_SRCLEVEL, ARG_SRCX, ARG_SRCY, _ARG_COUNT };
		enum{ SizeForFullArgs = 2 + _ARG_COUNT,
			ArgCombinations = 4,
		};
		const size_t argIndicesAll[ArgCombinations][_ARG_COUNT] = {
			{2,3,4,5,6,7,8,9},
			{2,3,0,4,5,6,7,8},
			{2,3,4,5,6,7,0,0},
			{2,3,0,4,5,6,0,0}
		};

		size_t nIdxFirstArgFailedToParse = 0;

		auto psw = GetDocI();

		const size_t (&argIndices)[_ARG_COUNT] = argIndicesAll[SizeForFullArgs - nArg];
		if ( ( nArg <= (SizeForFullArgs-ArgCombinations) || nArg > SizeForFullArgs )
			|| (argIndices[ARG_DESTLEVEL] == 0 && !psw)
			)
		{
			gEnv->pLog->LogError("Invalid argument combination");
			return bRet;
		}



		Vec2i vSize;
		vSize.x = _ParseInt(argIndices[ARG_WIDTH], pArgs, nIdxFirstArgFailedToParse);
		vSize.y = _ParseInt(argIndices[ARG_HEIGHT], pArgs, nIdxFirstArgFailedToParse);
		const char* szDestLevel;
		szDestLevel = ( (!argIndices[ARG_DESTLEVEL]) ? psw->GetWorldName() : pArgs->GetArg(argIndices[ARG_DESTLEVEL]));

		Vec2i destTopLeft;
		destTopLeft.x = _ParseInt(argIndices[ARG_DESTX], pArgs, nIdxFirstArgFailedToParse);
		destTopLeft.y = _ParseInt(argIndices[ARG_DESTY], pArgs, nIdxFirstArgFailedToParse);

		const char* szSrcLevel = pArgs->GetArg(argIndices[ARG_SRCLEVEL]);
		Vec2i srcTopLeft;
		srcTopLeft.x = ((!argIndices[ARG_SRCX]) ? 0: _ParseInt(argIndices[ARG_SRCX], pArgs, nIdxFirstArgFailedToParse));
		srcTopLeft.y = ((!argIndices[ARG_SRCY]) ? 0: _ParseInt(argIndices[ARG_SRCY], pArgs, nIdxFirstArgFailedToParse));

		if (nIdxFirstArgFailedToParse)
		{
			gEnv->pLog->LogError("Invalid argument at index(%d): %s", nIdxFirstArgFailedToParse, pArgs->GetArg(nIdxFirstArgFailedToParse));
			return bRet;
		}


		if (!MergeLevel(szDestLevel,szSrcLevel,destTopLeft,srcTopLeft, vSize))
		{
			gEnv->pLog->LogError("Level merging failed");
			return bRet;
		}

		gEnv->pLog->Log("Level merging succeeded");

		return bRet;
	}
	CMD("remove", "[level] <min x of bbox> <min y of bbox> <width> <height> - remove segments in specified area")
	{
		auto psw = GetDocI();
		size_t nOpt = pArgs->GetArgCount() - (_BASE_OFFSET+4);

		if ((pArgs->GetArgCount() != _BASE_OFFSET+4	&& pArgs->GetArgCount() != _BASE_OFFSET+5)
			|| (!nOpt && !psw) )
		{
			gEnv->pLog->LogError("Invalid argument combination");
			return bRet;
		}

		const char* szLevel = ( nOpt? pArgs->GetArg(_BASE_OFFSET+0) : psw->GetWorldName() );

		size_t nIdxFirstArgFailedToParse = 0;
		Recti rcArea;
		rcArea.Min.x = _ParseInt(_BASE_OFFSET+nOpt+0, pArgs, nIdxFirstArgFailedToParse);
		rcArea.Min.y = _ParseInt(_BASE_OFFSET+nOpt+1, pArgs, nIdxFirstArgFailedToParse);
		rcArea.Max.x = rcArea.Min.x + _ParseInt(_BASE_OFFSET+nOpt+2, pArgs, nIdxFirstArgFailedToParse);
		rcArea.Max.y = rcArea.Min.y + _ParseInt(_BASE_OFFSET+nOpt+3, pArgs, nIdxFirstArgFailedToParse);

		if (nIdxFirstArgFailedToParse)
		{
			gEnv->pLog->LogError("Invalid argument at index(%d): %s", nIdxFirstArgFailedToParse, pArgs->GetArg(nIdxFirstArgFailedToParse));
			return bRet;
		}

		if (!RemoveSegments(rcArea,szLevel))
		{
			gEnv->pLog->LogError("Failed to remove segments");
			return bRet;
		}

		gEnv->pLog->Log("Segments successfully removed.");
		return bRet;
	}
	CMD("boxes", "0|1|2|3 - control bounding boxes display")
	{
		if (nArg < 3) 
		{
			gEnv->pLog->LogToConsole("Draw boxes mode: %d", m_nDrawBoxes);
			return bRet;
		}
		m_nDrawBoxes = atoi(pArgs->GetArg(2));
		return bRet;
	}

#undef CMD

	if (!strcmpi(pcmd, "help"))
	{
		gEnv->pLog->LogToConsole("Usage: sw [command] [parameters]");
		gEnv->pLog->LogToConsole("Available SW manager commands:");
		for (int i = 0; i < nCmds; ++i)
			gEnv->pLog->LogToConsole("  %s %s", pcCmds[i], pcHelps[i]);
		return false;	// return as not handled so that this duty will pass on to other handlers in the chain.
	}

	return bCmdHandled;
}

bool CSegmentedWorldManager::MergeLevel(const CString& strDestLevel, 
										const CString& strSrcLevel, 
										const Vec2i& vDestTopLeft, 
										const Vec2i& vSrcTopLeft, 
										const Vec2i& vSize)
{
	CString strDestPath = Path::GamePathToFullPath(CString("Levels/"+strDestLevel));
	CString strSrcPath = Path::GamePathToFullPath(CString("Levels/"+strSrcLevel));
	Path::ConvertBackSlashToSlash(strDestPath);
	Path::ConvertBackSlashToSlash(strSrcPath);

	CSegmentLevelMerger levelMerger(strDestPath, strSrcPath, vDestTopLeft, vSrcTopLeft, vSize);

	auto inputIsValid = levelMerger.ValidateInput();
	if (!inputIsValid)
		return false;

	levelMerger.Process(true);

	//////////////////////////////////////////////////////////////////////////
	// TODO: do we need a report??
	GetISystem()->GetIPak()->OpenPack(strDestPath + "/"SW_LevelPakFile);

	return true;
}

bool CSegmentedWorldManager::RemoveSegments(const Recti& rcArea, const char* szWorldName /*= NULL*/ )
{
	std::vector<CPoint> wcs;
	int w = rcArea.GetWidth();
	int h = rcArea.GetHeight();
	for (int dy = 0; dy < h; ++dy)
		for (int dx = 0; dx < w; ++dx)
			wcs.push_back(CPoint(rcArea.Min.x + dx, rcArea.Min.y + dy));

	return RemoveSegments(wcs, szWorldName);
}

bool CSegmentedWorldManager::RemoveSegments(const std::vector<CPoint>& wcs, const char* szWorldName /*= NULL*/ )
{
	if (!szWorldName && GetIEditor()->IsValidSegmentedWorldDoc())
		szWorldName = GetIEditor()->GetSegmentedWorldDoc().GetWorldName();
	if (!szWorldName)
		return false;

	CString strLevelPath = Path::GamePathToFullPath(CString("Levels/") + szWorldName);
	Path::ConvertBackSlashToSlash(strLevelPath);

	CSegmentsRemover segRemover(strLevelPath, wcs);
	auto inputIsValid = segRemover.ValidateInput(strLevelPath, wcs);
	if (!inputIsValid)
		return false;

	segRemover.Process();

	// update SW_ObjectInfoFile and delete external objects ??
	// Update levelinfo.xml ??
	return true;
}

void CSegmentedWorldManager::GetMissingSegmentsInRect(const char *levelpath, const Recti &rect, std::list<Recti> &segRect, int segInMeters)
{
	CString strSegs;
	for(int j = rect.Min.y; j < rect.Max.y; j++)
	{
		for(int i = rect.Min.x; i < rect.Max.x; i++)
		{
			strSegs.Format("%s\\" SW_EditorDataDirPath "seg%d_%d" , levelpath, i, j);
			Path::ConvertBackSlashToSlash(strSegs);
			if(!CFileUtil::PathExists(strSegs))
				segRect.push_back(Recti(i, j, i+1, j+1) * segInMeters);
		}
	}
}

bool CSegmentedWorldManager::GetWorldBounds(const CString& sLevelpath, Recti& rect)
{
	CString strSrcPackFile = sLevelpath + "/"SW_LevelPakFile;
	CString sXML = sLevelpath + SW_SegmentedLevelInfo;

	gEnv->pCryPak->OpenPack(strSrcPackFile);

	XmlNodeRef xml = XmlHelpers::LoadXmlFromFile(sXML);
	if(!xml)
		return false;
	
	xml->getAttr("SegmentedWorldMinX", rect.Min.x);
	xml->getAttr("SegmentedWorldMinY", rect.Min.y);
	xml->getAttr("SegmentedWorldMaxX", rect.Max.x);
	xml->getAttr("SegmentedWorldMaxY", rect.Max.y);

	gEnv->pCryPak->ClosePack(strSrcPackFile);

	return true;
}

void CSegmentedWorldManager::OnSWDrawBoundingBoxVarChange( ICVar *pArgs )
{
	int val = pArgs->GetIVal();
	GetIEditor()->GetSegmentedWorldManager()->SetDebugDraw(val);
}

// WARNING: This function is used in DataUpgrade, don't forget to "Copy-on-change" to DataUpgrade module.
bool CSegmentedWorldManager::IsSWLevel( const char * szPathName )
{
	CString sPathName, sPath, sFilename, sFileExt;
	sPathName = szPathName;
	Path::Split(sPathName, sPath, sFilename, sFileExt);

	if ( 0 == sFileExt.CompareNoCase(".cry") )
		return false;

	if ( 0 == sFileExt.CompareNoCase(".scry") )
		return true;

	assert(0);
	SWLog("Unidentifiable level files, possible data corruption");
	return false;
}

bool CSegmentedWorldManager::SaveAsSWLevel(CCryEditDoc* pDoc, const char* szFullName, const Recti &rcWorld)
{
	GetIEditor()->GetSegmentedWorldManager()->SetScryPath(szFullName);

	CCryEditDoc::TSaveDocContext context;
	pDoc->BeforeSaveDocument(szFullName,context);
	
	auto pSWDoc = GetIEditor()->GetSegmentedWorldManager()->CreateNewDoc(szFullName, rcWorld);
	assert(pSWDoc);
	if (!pSWDoc)
		return FALSE;

	// Save Tag Point locations to file if auto save of tag points disabled
	if (!gSettings.bAutoSaveTagPoints)
		((CCryEditApp *)(AfxGetApp()))->SaveTagLocations();

	pSWDoc->Save(false);

	pDoc->AfterSaveDocument(szFullName,context);

	return true;
}

TSWDataStructureVersion CSegmentedWorldManager::GetSWDataStructureVersion()
{
	return g_nSWDataStructureVersion;
}

UINT CSegmentedWorldManager::GetSegmentSizeInUnits()
{
	return SEGMENT_SIZE_UNITS;
}

TSWDataStructureVersion CSegmentedWorldManager::MayUpgradeLevel( const char *pszLevelPath )
{
	TSWDataStructureVersion verLevel = CDataUpgrade::Get().DetectSWLevelVersion(pszLevelPath);
	if (verLevel == SWDStructVer_Invalid)
		return SWDStructVer_Invalid;

	SWLog("SWDataStructureVersion: %d",verLevel);

	if (verLevel < CSegmentedWorldManager::GetSWDataStructureVersion())
	{
		SWLog("Upgrading Level from version(%d) to version(%d)",verLevel, CSegmentedWorldManager::GetSWDataStructureVersion());
		CDataUpgrade::EUpgradeResult upgradeResult = CDataUpgrade::Get().Upgrade(&verLevel,pszLevelPath);

		switch (upgradeResult)
		{
		case CDataUpgrade::UR_FAILED:
			SWLog("Failed to upgrade level, stopped at version(%d)",verLevel);
			break;
		case CDataUpgrade::UR_BROKEN:
			SWLog("Failed to upgrade level, stopped at version(%d), and the level might be BROKEN", verLevel);
			break;
		}

		assert(verLevel == CSegmentedWorldManager::GetSWDataStructureVersion());
	}
	return verLevel;
}

sw::TPersistencePtr CSegmentedWorldManager::CreateTempPersistence( const char* pszLevelPath, bool bUseNullVC /*= false*/ )
{
	ISWVersionControl* pVCRaw;
	{
		m_nOnlineMode = 1;
		//CEngineSettingsManager settingsMgr;
		//settingsMgr.GetModuleSpecificIntEntry("EDT_SourceControl_Offline", bIsOffline);
		ISourceControl *pSCM = GetIEditor()->GetSourceControl();
		if(!GetIEditor()->GetSegmentedWorldManager()->IsMapConverting())
		{
			if(pSCM)
			{
				// Check if the opening level is managed
				CString strLevelFileName = pszLevelPath;
				strLevelFileName += GetWorldDataFileName(WDB_LEVELGENERAL);		// TODO: use WorldData::GetFileName instead
				uint32 nFileAttr = pSCM->GetFileAttributes(strLevelFileName );
				m_nOnlineMode = ((nFileAttr & SCC_FILE_ATTRIBUTE_MANAGED) != 0);
			}
		}
		else
			m_nOnlineMode = 0;

		if(!m_nOnlineMode || !pSCM)
			pVCRaw = new CNullVersionControl();
		else
			pVCRaw = new CNativeSCMAdapter(GetIEditor()->GetSegmentedWorldManager(), pSCM);
	}

	_smart_ptr<sw::ISWVersionControl> pVC = pVCRaw;
	return new CSCDataPersistence(pVC,pszLevelPath);
}

bool CSegmentedWorldManager::SaveLastEditInfoForLevel(const CString& sLevelName, 
													  const Recti* rcWorldRect /*= NULL*/,
													  int nSwZoomLvl /*= 0*/, 
													  const Vec2 &ptOfs /*= Vec2(0,0)*/)
{
	if (!CFileUtil::OverwriteFile(sLevelName))
		return false;

	CXmlArchive xmlAr;
	CPakFile pakFile;
	GetIEditor()->GetSystem()->GetIPak()->OpenPack(sLevelName);
	bool loadFromPakSuccess = xmlAr.LoadFromPak(Path::GetPath(Path::GetRelativePath(sLevelName)), pakFile);

	if (!loadFromPakSuccess)
	{
		xmlAr.root = XmlHelpers::CreateXmlNode("SegmentedWorld");
	}

	XmlNodeRef worldMapNode = xmlAr.root->findChild("WorldMap");
	if (!worldMapNode)
		worldMapNode = xmlAr.root->newChild("WorldMap");

	if (rcWorldRect != NULL)
	{
		xmlAr.root->setAttr("x", rcWorldRect->Min.x);
		xmlAr.root->setAttr("y", rcWorldRect->Min.y);
		xmlAr.root->setAttr("w", rcWorldRect->Max.x);
		xmlAr.root->setAttr("h", rcWorldRect->Max.y);

		xmlAr.root->setAttr("minX", m_pSWDocInternal->RcWorldBounds().Min.x);
		xmlAr.root->setAttr("minY", m_pSWDocInternal->RcWorldBounds().Min.y);
		xmlAr.root->setAttr("maxX", m_pSWDocInternal->RcWorldBounds().Max.x);
		xmlAr.root->setAttr("maxY", m_pSWDocInternal->RcWorldBounds().Max.y);

		worldMapNode->setAttr("ofs", ptOfs);
		worldMapNode->setAttr("zoom", nSwZoomLvl);
	}
	else
	{
		// set a default value
		xmlAr.root->setAttr("x", 0);
		xmlAr.root->setAttr("y", 0);
		xmlAr.root->setAttr("w", 1);
		xmlAr.root->setAttr("h", 1);
		worldMapNode->setAttr("ofs", Vec2(0,0));
		worldMapNode->setAttr("zoom", 2);
	}

	// close read-only access for writing
	GetIEditor()->GetSystem()->GetIPak()->ClosePack(sLevelName);

	if (!pakFile.Open(sLevelName, false))
		return false;

	bool bSaved = xmlAr.SaveToPak(Path::GetPath(sLevelName), pakFile);
	pakFile.Close();

	// re-open for read
	GetIEditor()->GetSystem()->GetIPak()->OpenPack(sLevelName);

	if (!bSaved)
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// struct CDeferredCommandQueue

void CDeferredCommandQueue::ExecOne()
{
	if (m_cmdQueue.empty())
	{
		// terminate once we finish the process in batch mode
		if(((CCryEditApp*)AfxGetApp())->IsInSWBatchMode())
			exit(0);

		return;
	}

	if (m_nDeferredCCPauseReqCount > 0)
		return;

	if (m_timeDeferredCCSleepUntil > gEnv->pTimer->GetAsyncTime())
		return;

	// maybe the current document is loading?
	if (!GetIEditor()->GetDocument()->IsDocumentReady())
		return;

	TDeferredCommand& cmd = m_cmdQueue.front();

	// =============  STEP 1 ===============
	// Sleep if needed
	if (cmd.timeSleep > 0LL)
	{
		m_timeDeferredCCSleepUntil = gEnv->pTimer->GetAsyncTime() + cmd.timeSleep;
		cmd.timeSleep = 0LL;
		return;
	}

	// =============  STEP 2 ===============

	if (cmd.IsNull())
	{
		m_cmdQueue.pop_front();
		return;
	}

	// if the level & area is not opened, open it now and execute command in next update
	string sWorldName = GetIEditor()->GetSegmentedWorldDoc().GetWorldName();
	bool bRequestNewLevel = (sWorldName.MakeLower() != cmd.sName.MakeLower());
	bool bRequestNewArea = !GetIEditor()->GetSegmentedWorldManager()->GetDocI()->GetLoadedRect().IsEqual(cmd.rcArea);
	if (GetIEditor()->GetSegmentedWorldDoc().IsNull() || bRequestNewLevel || bRequestNewArea)
	{
		// open the level 
		bool bOpenOK = true;
		CTimeValue timeSleep;
		if (GetIEditor()->GetSegmentedWorldDoc().IsNull() || bRequestNewLevel)
		{
			if ( OpenResult_NoError != GetIEditor()->GetSegmentedWorldManager()->OpenWorld(cmd.sName, true, &cmd.rcArea) )
				bOpenOK = false;

			// sleep a few seconds to let the level fully loaded
			timeSleep = 5.f;
		}

		// open area
		if (bOpenOK && bRequestNewArea)
		{
			if (!GetIEditor()->GetSegmentedWorldDoc().MoveTo(cmd.rcArea.Min.x, 
													cmd.rcArea.Min.y,
													cmd.rcArea.GetWidth(),
													cmd.rcArea.GetHeight(),
													true))
				bOpenOK = false;

			if (timeSleep <= 0LL)
				timeSleep = 3.f;		// sleep shorter if only move-to
		}

		// if open failed, remove this command
		if (!bOpenOK)
		{
			m_cmdQueue.pop_front();
			return;
		}

		this->Sleep_(timeSleep);
		return;
	}

	// =============  STEP 3 ===============

	// execute the command then pop it
	GetISystem()->GetIConsole()->ExecuteString(cmd.strCmd);
	m_cmdQueue.pop_front();
}

void CDeferredCommandQueue::Sleep_( CTimeValue timeIn )
{
	if (m_cmdQueue.empty())
		m_cmdQueue.push_back(TDeferredCommand());	// push a null command to hold the sleep value

	m_cmdQueue.front().timeSleep += timeIn;
}

