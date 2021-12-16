#include "StdAfx.h"
#include "InternalCommon.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorldDoc.h"
//#include "DBWrapper.h"
#include "SCDataPersistence.h"
#include "NullVersionControl.h"
#include "NativeSCMAdapter.h"
#include "DataUpgrade.h"

// UIs
#include "GridMapDlg.h"
#include "SegmentedWorldCommitDlg.h"
#include "SegmentedWorldNewWorldDlg.h"

#include <ISystem.h>
#include "Terrain/TerrainManager.h"
#include "./Objects/Group.h"
#include "Objects/EntityObject.h"
#include "./HyperGraph/HyperGraph.h"
#include "./HyperGraph/FlowGraph.h"
#include "./HyperGraph/FlowGraphManager.h"
#include "./Brush/SolidBrushObject.h"
#include "CryEditDoc.h"
#include "ViewPane.h"
#include "Util/PakFile.h"
#include <EngineSettingsManager.h>
#include "ISourceControl.h"


using namespace sw;


CSegmentedWorldManager CSegmentedWorldManager::s_instance;


class CSWSingleDocTemplate : public CCrySingleDocTemplate
{
public:
	CSWSingleDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass,CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass, const char* szOverrideDocStrings = NULL)
		:CCrySingleDocTemplate(nIDResource,pDocClass,pFrameClass,pViewClass)
	{
		if (szOverrideDocStrings)
			this->m_strDocStrings = szOverrideDocStrings;
	}

	// Match SW Doc by overriding the match function, because file extension checking is not enough for it..
	virtual CDocTemplate::Confidence MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch)
	{
		ASSERT(lpszPathName != NULL);
		rpDocMatch = NULL;

		// go through all opened documents
		POSITION pos = GetFirstDocPosition();
		while (pos != NULL)
		{
			CDocument* pDoc = GetNextDoc(pos);
			
			if (0 == stricmp(pDoc->GetPathName(), lpszPathName))
			{
				// already open
				rpDocMatch = pDoc;
				return yesAlreadyOpen;
			}
		}

		if (CSegmentedWorldManager::IsSWLevel(lpszPathName))
			return yesAttemptNative;

		// otherwise we will guess it may work
		return yesAttemptForeign;
	}
};


//-----------------------------------------------------------------------------------------------
void CSegmentedWorldManager::ConsoleCommandEntry(IConsoleCmdArgs *pArgs)
{
	// non doc commands  (SW module commands)
	bool bHandled = CSegmentedWorldManager::Get().ConsoleCommandHandler(pArgs);
	if (bHandled)
		return;

	CSWDoc* pswdoc = CSegmentedWorldManager::Get().GetDocI();
	if (pswdoc)
	 pswdoc->ConsoleCommand(pArgs);
}


CSegmentedWorldManager::CSegmentedWorldManager()
	:m_pSWDoc(NULL)
	,m_pSWDocInternal(NULL)
	,m_pDocTemplate(NULL)
{
	m_nSWMode = 0;
	m_nDrawBoxes = 1;
	s_nIgnoringChanges = 0;
}

//! Initializer for the manager
void CSegmentedWorldManager::Init()
{
	// SW Doc template
	CString strSWDocStrings;
	strSWDocStrings.LoadStringA(IDR_MAINFRAME_SW);
	CSingleDocTemplate* pDocTemplate = new CSWSingleDocTemplate(
		IDR_SELECTED_MENU,
		RUNTIME_CLASS(CSWDocAdapter),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CLayoutViewPane),
		strSWDocStrings);
	AfxGetApp()->AddDocTemplate(pDocTemplate);
	m_pDocTemplate = pDocTemplate;

	// register console commands and cvars
	REGISTER_COMMAND( "sw", &CSegmentedWorldManager::ConsoleCommandEntry, VF_NULL, "Control segmented world, use 'sw help' for more info" );
	RegisterConsoleVariables();
	GetIEditor()->RegisterNotifyListener(this);
}

void CSegmentedWorldManager::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnIdleUpdate:
		if (m_pSWDoc)
			m_pSWDoc->UpdateOnIdle();

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
	fullName.Format("%s/%s.cry", fullPathName.c_str(), worldname.c_str());

	if(PathFileExists(fullName.c_str()))
	{
		AfxMessageBox( _T("Level with this name already exists, choose another name."), MB_ICONEXCLAMATION|MB_OK );
		return false;
	}

	gEnv->pConsole->GetCVar("doc_enable_segmented_world")->Set(1);
	gEnv->pConsole->GetCVar("doc_segmented_world_mode")->Set(1);	
	CSegmentedWorldDoc::s_bAutomate = true;
	CSegmentedWorldDoc::s_bMapConverting = true;
	bool bRes = false;
	//IDD_SW_SAVE_NAME
	//TODO
	//TODO
	
	CCryEditDoc *doc = 0;
	doc = (CCryEditDoc*)GetIEditor()->GetDocument();
	ASSERT_KINDOF(CCryEditDoc,doc);

	CRect rcWorld;
	{
		CHeightmap* pHMap = GetIEditor()->GetHeightmap();
		int w = pHMap->GetHeight() / SEGMENT_SIZE_UNITS;
		int h = pHMap->GetWidth() / SEGMENT_SIZE_UNITS;

		rcWorld.TopLeft().SetPoint(0,0);
		rcWorld.BottomRight().SetPoint(w,h);
	}


	// --== STEP-1: Generate and fill all SW structures.	==--
	//bRes = doc->OnSaveDocument(fullName.c_str());
	if (!SaveAsSWLevel(doc, fullName))
		return false;


	// --== STEP-2: Open it as SW level						==--
	//GetDocI()->worlddata_Unapply(); // because we will have to apply it again
	TWorldName currWorld;
	currWorld.Name.Format("%s",GetDocI()->GetWorldName());
	currWorld.isSegmented = true;
	//GetIEditor()->GetDocument()->OnCloseDocument();
	OpenWorld(currWorld, false, &rcWorld);

	// --== STEP-3: Re-generate surface texture				==--
	((CCryEditApp*)AfxGetApp())->GenerateSurfaceTexture();


	// --== STEP-4: Save all as SW level					==--
	GetISystem()->GetIConsole()->ExecuteString("sw gd lock");
	doc = GetIEditor()->GetDocument();
	string name = GetIEditor()->GetLevelFolder() + "\\" + GetDocI()->GetWorldName();
	if(doc->DoSave(name,TRUE))
		bRes = bRes && true;
	
	// --== STEP-5: Reopen as SW level???					==--
	OpenWorld(currWorld, false, &rcWorld);

	CSegmentedWorldDoc::s_bAutomate = false;

	// TODO: implement 'commit'
	//GetISystem()->GetIConsole()->ExecuteString("sw commit");

	//GetDocI()->GenerateSegmentedMap();
	SetCurrentDirectoryW( GetIEditor()->GetMasterCDFolder() );

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
	m_pSWDocInternal = NULL;

	// disable all the sw flags
	//gEnv->pConsole->GetCVar("doc_enable_segmented_world")->Set(0);
	//gEnv->pConsole->GetCVar("doc_segmented_world_mode")->Set(0);
	//CSegmentedWorldDoc::s_bAutomate = false;
	//CSegmentedWorldDoc::s_bMapConverting = false;
}

CSWManager &CSegmentedWorldManager::Get()
{
	return CSegmentedWorldManager::s_instance;
}

bool CSegmentedWorldManager::IsActive()
{
	return GetIEditor()->Get3DEngine()->IsSegmentedWorldActive();
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
	return m_nSWMode == 0;
}

bool CSegmentedWorldManager::IsMapConverting()
{
	return CSegmentedWorldDoc::s_bMapConverting;
}

void CSegmentedWorldManager::OnModeChange(ICVar* pVar)
{
	s_instance.DoOnModeChange(pVar);
}

void CSegmentedWorldManager::DoOnModeChange(ICVar* pVar)
{
	AUTO_LOCK_BIG(g_csBigLock);
	CSegmentedWorldDoc* psw;
	psw = GetDocI();
	static bool s_bQuiet = false;

	if (s_bQuiet)
	{
		m_nSWMode = pVar->GetIVal();
		if (CGridMapDlg::Get())
			CGridMapDlg::Get()->OnSWModeChange(m_nSWMode);
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
							pVar->Set(m_nSWMode);
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
					pVar->Set(m_nSWMode);
					s_bQuiet = false;
					return;
				}
			}
		}
		else if (m_nSWMode != 0 && pVar->GetIVal() == 0)
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
					pVar->Set(m_nSWMode);
					s_bQuiet = false;
					return;
				}
			}
		}
	}
	m_nSWMode = pVar->GetIVal();
	if (CGridMapDlg::Get())
		CGridMapDlg::Get()->OnSWModeChange(m_nSWMode);
	SWLog("SWMode changed\n"); 
}

void CSegmentedWorldManager::GetPakPath(CString &strSegmentPakFileName, CString &strSegmentPakPath, CString &strSegmentFileBlockPath, int wx, int wy, bool bMultiPak, bool bCreateDir)
{
	Get().GetDocI()->GetPersistence()->GetPakPath(strSegmentPakFileName, strSegmentPakPath, strSegmentFileBlockPath, TWorldCoords(wx, wy), bMultiPak, bCreateDir);
}

bool CSegmentedWorldManager::IsLevelInMultiPack()
{
	return s_bLevelMultiPack;
}

ISegmentedWorldDoc* CSegmentedWorldManager::CreateNewDoc( const char *szPath )
{
	assert(szPath);
	if (!szPath)
		return NULL;

	if (m_pSWDoc)
	{
		assert(0);
		DeleteDoc();
	}
	m_pSWDocInternal = new CSegmentedWorldDoc();

	CRGBLayer &TerrainRGBLayer = CTerrainManager::GetTerrainManager().GetHeightmap().m_TerrainRGBTexture;
	CHeightmap *phm = GetIEditor()->GetHeightmap();
	if (phm) {
		phm->ClearModSectors();
		int w = phm->GetHeight() / SEGMENT_SIZE_UNITS;
		int h = phm->GetWidth() / SEGMENT_SIZE_UNITS;
		if (w < 1)
			w = 1;
		if (h < 1)
			h = 1;
		Seg2RGB(w, h);
		if (w != TerrainRGBLayer.GetTileCountX() || h != TerrainRGBLayer.GetTileCountY())
			TerrainRGBLayer.Resize(w, h, SEGMENT_SIZE_UNITS);

		//Generating world map if not exists
		//CString sWorldMapFile = PathUtil::GetPath(pcPath) + "world-map.bmp";
		//CFile tmp;
		//if (tmp.Open(sWorldMapFile, CFile::modeRead | CFile::typeBinary)) {
		//	tmp.Close();
		//} else {
		//	CImage img;
		//	img.Allocate(512, 512);
		//	TerrainRGBLayer.GetSubImageStretched(	0 , 0, 1 , 1, img, true);
		//	img.SwapRedAndBlue();
		//	CImageUtil::SaveBitmap(sWorldMapFile, img);
		//}
	}
	//TerrainRGBLayer.LoadAll();

	if (gEnv->pConsole->GetCVar("doc_segmented_world_mode"))
		m_nSWMode = gEnv->pConsole->GetCVar("doc_segmented_world_mode")->GetIVal();

	// re-create VC & persistence if offline mode changed
	TPersistencePtr pPersist = CreateTempPersistence( Path::GetRelativePath(Path::GetPath(szPath)));

	if (!m_pSWDocInternal->Init(false, szPath, true,pPersist))
	{
		assert(0);
		SAFE_DELETE(m_pSWDocInternal);
		return NULL;
	}

	// pass it to public accessible pointer only when it's initialized
	m_pSWDoc = m_pSWDocInternal;

	if (CGridMapDlg::Get())
		CGridMapDlg::Get()->OnLevelChanged();

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

	if (gEnv->pConsole->GetCVar("doc_segmented_world_mode"))
		m_nSWMode = gEnv->pConsole->GetCVar("doc_segmented_world_mode")->GetIVal();

	// TODO: re-create VC only if level or offline mode changed
	TPersistencePtr pPersistTmp = CreateTempPersistence( Path::GetRelativePath(Path::GetPath(szPath)));

	// upgrade data if needed
	if (MayUpgradeLevel(pPersistTmp) != GetSWDataStructureVersion()
		|| !m_pSWDocInternal->Init(true, szPath, bForce, pPersistTmp))
	{
		assert(0);
		SAFE_DELETE(m_pSWDocInternal);
		return ISegmentedWorldDoc::GetNullObject();
	}


	// publish it after initialized
	m_pSWDoc = m_pSWDocInternal;

	if (CGridMapDlg::Get())
		CGridMapDlg::Get()->OnLevelChanged();

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
	if (Get().GetDocI() != NULL)
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
			CSWManager::SWMsgBox(SWMsgType_AccessDenied_Graph, pObject);
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
				CSWManager::SWMsgBox(SWMsgType_AccessDenied_Graph, pObject);

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

void CSegmentedWorldManager::FindLocalWorlds(std::vector<TWorldName> &lstNames)
{
	CString sPath = gEnv->pCryPak->GetGameFolder() + string("/levels");
	
	ICryPak *pCryPak = gEnv->pCryPak;
	_finddata_t fd;

  char chMaskDir[1024]; sprintf(chMaskDir, "%s/*", sPath);

  intptr_t handle = pCryPak->FindFirst(chMaskDir, &fd, 0);
  if (handle < 0)
    return;

  do {
    if (*fd.name && *fd.name != '.') {
      //Directory
      if (fd.attrib & _A_SUBDIR) {
        char chFileName[1024]; sprintf(chFileName, "%s/%s", sPath, fd.name);
        
				// check for segmented world
				_finddata_t fd3;
				char chMaskSWFile[1024]; sprintf(chMaskSWFile, "%s/sw.xml", chFileName);
				intptr_t handle3 = pCryPak->FindFirst(chMaskSWFile, &fd3, 0);
				// if not SW search for *.cry
				if (handle3 < 0) {
					_finddata_t fd2;
					char chMaskCryFile[1024]; sprintf(chMaskCryFile, "%s/*.cry", chFileName);
					intptr_t handle2 = pCryPak->FindFirst(chMaskCryFile, &fd2, 0);
					if (handle2 < 0)
						continue;

					do {
						if(*fd2.name && *fd2.name != '.') {
							if (!(fd2.attrib & _A_SUBDIR)) {
								char chCryFileName[1024]; sprintf(chCryFileName, "%s/%s", fd.name, fd2.name);
								TWorldName wnCurr;
								wnCurr.Name = chCryFileName;
								wnCurr.isSegmented = false;
								lstNames.push_back(wnCurr);
							}
						}
					} while (pCryPak->FindNext(handle2, &fd2) >= 0);

					pCryPak->FindClose(handle2);
				} else {
					TWorldName wdName;
					wdName.isSegmented = true;
					wdName.Name = fd.name;
					lstNames.push_back(wdName);

					pCryPak->FindClose(handle3);
				}
      }
      //File
			else {
				// nothing for now
			}
    }
  } while (pCryPak->FindNext(handle, &fd) >= 0);

  pCryPak->FindClose(handle);
}


void CSegmentedWorldManager::ListWorlds(std::vector<TWorldName> &lstNames, int nLevelListType)
{
	if (nLevelListType == LevelList_All || nLevelListType == LevelList_Local)
	{
		FindLocalWorlds(lstNames);
	}
}

void CSegmentedWorldManager::OnOpenWorld()
{
	CString docFilename;
	CString currentLevelsPath = Path::GetExecutableParentDirectory() + "\\" + Path::GetGameFolder() + "\\Levels";
	if(CFileUtil::SelectFile("sw.xml", currentLevelsPath, docFilename))
	{
		GetIEditor()->GetDocument()->SetDocumentReady(false);
		AfxGetApp()->OpenDocumentFile( docFilename );

		gEnv->pConsole->GetCVar("doc_enable_segmented_world")->Set(1);
		gEnv->pConsole->GetCVar("doc_segmented_world_mode")->Set(1);
		CSegmentedWorldDoc::s_bAutomate = true; // CS - fix it!
		CSegmentedWorldDoc::s_bMapConverting = false;
	}
}

static string s_strLastOpenLevel_Local = 0;
static string s_strLastOpenLevel_SW = 0;
static string s_strLastOpenLevel_All = 0;

sw::EOpenResult CSegmentedWorldManager::OpenWorld( const TWorldName &wnName, bool bReopenOnly /*= false*/, const CRect* rcWorldRect /*= NULL*/, int nSwZoomLvl /*= 0*/, const Vec2 &ptOfs /*= Vec2(0,0)*/ )
{
	AUTO_LOCK_BIG(g_csBigLock);
	CDocument *doc = 0;
	doc = GetIEditor()->GetDocument();
	string strCurrDocName;
	string strNewDocName;
	string strParentDir;
	if (doc)
	{
		strCurrDocName = doc->GetPathName();
		strCurrDocName = PathUtil::GetFile(strCurrDocName.c_str());
		strParentDir = PathUtil::GetParentDirectory(string(doc->GetPathName()));
		strParentDir = PathUtil::GetFile(strParentDir.c_str());
		strCurrDocName = strParentDir + "/" + strCurrDocName;
	}
	strNewDocName = wnName.Name;
	if (wnName.isSegmented)
	{
		strNewDocName = strNewDocName + "/" + wnName.Name;
	}

	if (strCurrDocName.compare(strNewDocName) == 0 && ((GetDocI() != 0) == wnName.isSegmented))
	{
		//if is the same document;
		if(doc)
		{
			GetIEditor()->GetDocument()->SetDocumentReady(false);
			AfxGetApp()->OpenDocumentFile(doc->GetPathName());
			return OpenResult_AlreadyOpen;
		}
	}

	CString sPath = gEnv->pCryPak->GetGameFolder() + string("/levels/");
	if (wnName.isSegmented) {
		sPath = gEnv->pCryPak->GetGameFolder() + string("/levels/");
		sPath += wnName.Name;

		char configPath[256];
		sprintf(configPath, "game/levels/%s/level.cfg", wnName.Name.c_str());
		if(GetISystem()->GetIPak()->IsFileExist( configPath ))
  		GetISystem()->LoadConfiguration( configPath );

		if (!CFileUtil::PathExists(sPath))
		{
			if (bReopenOnly)
				return OpenResult_NotCached;
			CFileUtil::CreateDirectory(sPath);
		}

		sPath += "/";

		CString sXML = sPath + "sw.xml";
		XmlNodeRef sw = NULL, wm = NULL;

		if (!CFileUtil::FileExists(sXML))
			sw = XmlHelpers::CreateXmlNode("SegmentedWorld");
		else
			sw = XmlHelpers::LoadXmlFromFile(sXML);

		if (!sw)
			return OpenResult_OutOfMemory;

		if(!(wm = sw->findChild("WorldMap")))
			wm = sw->newChild("WorldMap");

		if (rcWorldRect != NULL)
		{
			sw->setAttr("x", rcWorldRect->left);
			sw->setAttr("y", rcWorldRect->top);
			sw->setAttr("w", rcWorldRect->Width());
			sw->setAttr("h", rcWorldRect->Height());
			wm->setAttr("ofs", ptOfs);
			wm->setAttr("zoom", nSwZoomLvl);
		}
		else
		{
			// set a default value
			sw->setAttr("x", 0);
			sw->setAttr("y", 0);
			sw->setAttr("w", 1);
			sw->setAttr("h", 1);
			wm->setAttr("ofs", Vec2(0,0));
			wm->setAttr("zoom", 2);
		}

		if (!sw->saveToFile(sXML))
			return OpenResult_FileOpError;

		s_strLastOpenLevel_SW.clear();
		s_strLastOpenLevel_SW.append(wnName.Name);
	} else {
		s_strLastOpenLevel_Local.clear();
		s_strLastOpenLevel_Local.append(wnName.Name);
	}

	s_strLastOpenLevel_All.clear();
	s_strLastOpenLevel_All.append(wnName.Name);

	sPath += wnName.Name;

	GetIEditor()->GetDocument()->SetDocumentReady(false);
	doc = AfxGetApp()->OpenDocumentFile(sPath);

	return OpenResult_NoError;
}

void CSegmentedWorldManager::GetLastOpenedWorld(string &strName, int nLevelListType)
{
	strName.clear();
	
	if (nLevelListType == LevelList_Local) {
		strName.append(s_strLastOpenLevel_Local);
	} else if (nLevelListType == LevelList_SW) {
		strName.append(s_strLastOpenLevel_SW);
	} else {
		strName.append(s_strLastOpenLevel_All);
	}
}

const char* CSegmentedWorldManager::GetVersionTypeName( sw::EVersionType vt )
{
	return g_pcVersionTypes[vt];
}

const char* CSegmentedWorldManager::GetSDBlockName( sw::ESDBType sdbt )
{
	return sw::GetSegDataBlockName(sdbt);
}

const char* CSegmentedWorldManager::GetWDBlockName( sw::EWDBType wdbt )
{
	return sw::GetWorldDataBlockName(wdbt);
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

		const char* szLevelName = pArgs->GetArg(2);
		CString strLevelPath = PathUtil::GetGameFolder() + "/levels/" + szLevelName +"/";
		if (!this->IsSWLevel(strLevelPath + szLevelName))
			return bRet;

		XmlNodeRef xmlWorldBounds;
		XmlNodeRef xmlSegs = XmlHelpers::LoadXmlFromFile(strLevelPath + "segmentinfo.xml");
		if (!xmlSegs || !xmlSegs->isTag("SegmentInfo") || !(xmlWorldBounds = xmlSegs->findChild("WorldBounds")))
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

		Vec2 vMin,vMax;
		xmlWorldBounds->getAttr("Min",vMin);
		xmlWorldBounds->getAttr("Max",vMax);
		CRect rcWorld(vMin.x,vMin.y,vMax.x+1,vMax.y+1);

		const int nStepSize = 8;		// tweak this
		// remain should take all main part if there are no gain on the other axis
		const int nXRemain = rcWorld.Width() % nStepSize;
		const int nYRemain = rcWorld.Height() % nStepSize;
		const int nXMainPartEnd = rcWorld.right - nXRemain;
		const int nYMainPartEnd = rcWorld.bottom - nYRemain;

		// command prototype
		CDeferredCommandQueue::TDeferredCommand defcmd;
		defcmd.timeSleep = 2.f;								// the interval between two commands
		defcmd.wname.Name = szLevelName;
		defcmd.wname.isSegmented = true;
		defcmd.strCmd = strCmd;

		// big slices
		for (int y = rcWorld.top; y < nYMainPartEnd; y += nStepSize )
		for (int x = rcWorld.left; x < nXMainPartEnd; x += nStepSize )
		{
			defcmd.rcArea.SetRect(CPoint(x,y), CPoint(x,y)+CSize(nStepSize,nStepSize));
			m_defCmdQueue.AddCommand(defcmd);
		}

		// slices on edges
		int nMinStep;
		// edge slices parallel to x-axis AND THE CORNER
		if (nMinStep = RightMostBit(nYRemain))
		{
			for (int y = nYMainPartEnd; y < rcWorld.bottom; y += nMinStep)
				for (int x = rcWorld.left; x < nXMainPartEnd; x += nMinStep)
				{
					defcmd.rcArea.SetRect(CPoint(x,y), CPoint(x,y)+CSize(nMinStep,nMinStep));
					m_defCmdQueue.AddCommand(defcmd);
				}
		}

		// edge slices parallel to y-axis WITHOUT CORNER
		if (nMinStep = RightMostBit(nXRemain))
		{
			for (int x = nXMainPartEnd; x < rcWorld.right; x += nMinStep)
				for (int y = rcWorld.top; y < nYMainPartEnd; y += nMinStep)
				{
					defcmd.rcArea.SetRect(CPoint(x,y), CPoint(x,y)+CSize(nMinStep,nMinStep));
					m_defCmdQueue.AddCommand(defcmd);
				}
		}

		// the corner
		if (nMinStep = std::min(RightMostBit(nXRemain),RightMostBit(nYRemain)))
		{
			for (int y = nYMainPartEnd; y < rcWorld.bottom; y += nMinStep)
				for (int x = nXMainPartEnd; x < rcWorld.right; x += nMinStep)
				{
					defcmd.rcArea.SetRect(CPoint(x,y), CPoint(x,y)+CSize(nMinStep,nMinStep));
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

		CSWDoc *psw = GetDocI();

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
	CMD("remove", "[level] <x of topleft corner> <y of topleft corner> <width> <height> - remove segments in specified area")
	{
		CSWDoc *psw = GetDocI();
		size_t nOpt = pArgs->GetArgCount() - (_BASE_OFFSET+4);

		if ((pArgs->GetArgCount() != _BASE_OFFSET+4	&& pArgs->GetArgCount() != _BASE_OFFSET+5)
			|| (!nOpt && !psw) )
		{
			gEnv->pLog->LogError("Invalid argument combination");
			return bRet;
		}

		const char* szLevel = ( nOpt? pArgs->GetArg(_BASE_OFFSET+0) : psw->GetWorldName() );

		size_t nIdxFirstArgFailedToParse = 0;
		CPoint ptTopLeft;
		ptTopLeft.x = _ParseInt(_BASE_OFFSET+nOpt+0, pArgs, nIdxFirstArgFailedToParse);
		ptTopLeft.y = _ParseInt(_BASE_OFFSET+nOpt+1, pArgs, nIdxFirstArgFailedToParse);
		CSize szArea;
		szArea.cx = _ParseInt(_BASE_OFFSET+nOpt+2, pArgs, nIdxFirstArgFailedToParse);
		szArea.cy = _ParseInt(_BASE_OFFSET+nOpt+3, pArgs, nIdxFirstArgFailedToParse);

		if (nIdxFirstArgFailedToParse)
		{
			gEnv->pLog->LogError("Invalid argument at index(%d): %s", nIdxFirstArgFailedToParse, pArgs->GetArg(nIdxFirstArgFailedToParse));
			return bRet;
		}

		if (!RemoveSegments(CRect(ptTopLeft,szArea),szLevel))
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
	CMD("test","<world_name>")
	{
		if (pArgs->GetArgCount() < 3)
			return bRet;

		TWorldName wn;
		wn.isSegmented = true;
		wn.Name = pArgs->GetArg(2);
		CRect rcArea;
		for (int y=0;y < 2; ++y)
			for (int x=0; x < 2; ++x)
			{
				OpenWorld(wn,true, &CRect( CPoint(x*2,y*2),CSize(2,2)));
				((CCryEditApp*)AfxGetApp())->ExportToGame();
			}
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

bool CSegmentedWorldManager::MergeLevel( CString const &strDestLevel, CString const& strSrcLevel, Vec2i const& vDestTopLeft, Vec2i const& vSrcTopLeft, Vec2i const& vSize )
{
	typedef std::set<GUID, guid_less_predicate> TGuidSet;
	enum{ SourceData_Dest, SourceData_Src, SourceData_count };
	Vec2i vDiffSrcToDest = vDestTopLeft - vSrcTopLeft;
	TRectI rectDest, rectSrc;
	rectDest.Min = vDestTopLeft;
	rectDest.Max = rectDest.Min + vSize - Vec2i(1,1);
	rectSrc.Min = vSrcTopLeft;
	rectSrc.Max = rectSrc.Min + vSize - Vec2i(1,1);		// the TRectI uses close range(not half open range)
	assert(TRectI(0,0,0,0).InRect(Vec2i(0,0)));			// as long as this assert is true, we need to minus 1 in Max

	CString strDestPath = "Levels/"+strDestLevel;
	CString strSrcPath = "Levels/"+strSrcLevel;
	strDestPath = Path::GamePathToFullPath(strDestPath);
	strSrcPath = Path::GamePathToFullPath(strSrcPath);
	Path::ConvertBackSlashToSlash(strDestPath);
	Path::ConvertBackSlashToSlash(strSrcPath);

	if (!CFileUtil::PathExists(strDestPath) || !CFileUtil::PathExists(strSrcPath))
	{
		SWLog("Destination or source level not exists");
		return false;
	}

	if (GetIEditor()->GetDocument()->IsDocumentReady() && GetDocI()) // if a SW level is opened check if it conflicts with this operation
	{
		CSWDoc *psw = GetDocI();
		CString strCurLevel = Path::GamePathToFullPath(GetIEditor()->GetLevelFolder());
		Path::ConvertBackSlashToSlash(strCurLevel);

		TRectI rectInUse;
		rectInUse.Min.set(psw->m_wcOfs.wx, psw->m_wcOfs.wy);
		rectInUse.Max = rectInUse.Min + Vec2i(psw->m_uiWidth,psw->m_uiHeight) - Vec2i(1,1);

		if (0==strDestPath.CompareNoCase(strCurLevel) && rectDest.Intersects(rectInUse))
		{
			SWLog("You cannot merge to the dest segment(s) that have been opened for editing");
			return false;
		}
		
		if (0==strSrcPath.CompareNoCase(strCurLevel) && rectSrc.Intersects(rectInUse))
		{
			SWLog("You cannot merge from the src segment(s) that have been opened for editing");
			return false;
		}
	}

	// Check level.pak availability
	//ICryPak* pIPak = GetISystem()->GetIPak();
	CPakFile srcPak, destPak;
	CString strSrcPackFile = strSrcPath + "/"SW_LevelPakFile;
	CString strDestPackFile = strDestPath + "/"SW_LevelPakFile;
	if (!srcPak.OpenForRead(strSrcPackFile)	// should be read-only
		|| !destPak.Open(strDestPackFile, true))
	{
		SWLog("Failed to open level.pak while merging levels");
		return false;
	}
	srcPak.Close();
	destPak.Close();

	// Check data structure version
	if (CDataUpgrade::Get().DetectSWLevelVersion(CreateTempPersistence(strDestPath)) != CSWManager::GetSWDataStructureVersion()
		|| CDataUpgrade::Get().DetectSWLevelVersion(CreateTempPersistence(strSrcPath)) != CSWManager::GetSWDataStructureVersion())
	{
		SWLog("Destination or source level is not in new format, please upgrade the level(s) first");
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Check HeightMap UnitSize, WaterLevel, TextureSize
	bool bMergeCoverCTC = true;
	{
		XmlNodeRef xmlDest = XmlHelpers::LoadXmlFromFile( strDestPath + "/" + g_szWDBNodeFileNames[WDB_LEVELGENERAL] );
		XmlNodeRef xmlSrc = XmlHelpers::LoadXmlFromFile( strSrcPath + "/" + g_szWDBNodeFileNames[WDB_LEVELGENERAL] );
		if (!(xmlDest && xmlDest->isTag("Level") && (xmlDest = xmlDest->findChild("Heightmap"))
			&& xmlSrc && xmlSrc->isTag("Level") && (xmlSrc = xmlSrc->findChild("Heightmap"))
			))
		{
			SWLog("Destination or source level seems to be broken! Merging aborted!");
			return false;
		}

		int nUnitSizeDest = 0, nUnitSizeSrc = 0;
		if (!(xmlDest->getAttr("UnitSize",nUnitSizeDest)
			&& xmlSrc->getAttr("UnitSize",nUnitSizeSrc)
			&& nUnitSizeDest == nUnitSizeSrc)
			)
		{
			SWLog("Destination and source level are having different HeightMap UnitSize! Merging aborted!");
			return false;
		}

		float nWaterLevelDest =0, nWaterLevelSrc = 0;
		if (!(xmlDest->getAttr("WaterLevel",nWaterLevelDest) && xmlSrc->getAttr("WaterLevel",nWaterLevelSrc)
			&& nWaterLevelDest == nWaterLevelSrc))
		{
			int nAnswer = SWMsgBoxQuestion("Warning: Destination and source level are having different WaterLevel, some part of terrain may going to be under water.\n"
								"Continue?");
			if (nAnswer != IDYES)
				return false;
		}
		
	}

	//////////////////////////////////////////////////////////////////////////
	// Check if any dest seg folders already exist
	//		the world allows holes, so width and height of world are useless
	// (we must do the whole check before copying, because we cannot rollback if anything conflicted)

	for (int dy = 0; dy < vSize.y; ++dy)
	{
		for (int dx = 0; dx < vSize.x; ++dx)
		{
			char tmpPath[MAX_PATH];
			_snprintf(tmpPath,MAX_PATH,"%s/" SW_EditorDataDirPath SW_SegmentDirPattern, strDestPath, vDestTopLeft.x + dx, vDestTopLeft.y + dy);
			if (CFileUtil::PathExists(tmpPath))
			{
				SWLog("Dest segment(s) already exists");
				return false;
			}
		}
	}

	CWaitProgress progressBar( _T("Merging Segments ...") );
	progressBar.Start();
	const int nProgStageEstimate = vSize.x * vSize.y;
	const int nProgressStageCount = 9;
	int nTotoalSteps = nProgressStageCount*nProgStageEstimate +1;
	int nCurStep = 0;

	progressBar.Step( 100*nCurStep++/nTotoalSteps );

	CString destEditorSegsRoot = strDestPath + "/"SW_EditorDataDirPath;
	CString srcEditorSegsRoot = strSrcPath + "/"SW_EditorDataDirPath;
	CString destSegsRoot = strDestPath + "/"SW_SegmentsDirPath;
	CString srcSegsRoot = strSrcPath + "/"SW_SegmentsDirPath;
	//////////////////////////////////////////////////////////////////////////
	// Merge Main Layers (before copying)
	// it's impossible to skip a certain file during the next step, so we have to keep the data
	// in memory and write it to file later.

	std::vector<std::pair<CPoint, XmlNodeRef> > arrMainLayerFragments;	// keep world coord, because holes are allowed
	arrMainLayerFragments.reserve(vSize.x*vSize.y);
	for (int dy = 0; dy < vSize.y; ++dy)
	{
		for (int dx = 0; dx < vSize.x; ++dx)
		{

			CString srcSegDir;
			srcSegDir.Format(SW_SegmentDirPattern, vSrcTopLeft.x + dx, vSrcTopLeft.y + dy);

			CString strFileSrc = srcEditorSegsRoot + srcSegDir + "/Main.lyr";
			uint64 uFilesize;
			if (CFileUtil::FileExists(strFileSrc) && CFileUtil::GetDiskFileSize(strFileSrc,uFilesize) && uFilesize > 0)
			{
				XmlNodeRef xmlResult = XmlHelpers::LoadXmlFromFile(strFileSrc);
				if (xmlResult && xmlResult->getChildCount() > 0)
					arrMainLayerFragments.push_back(std::make_pair(CPoint(vDestTopLeft.x + dx, vDestTopLeft.y + dy), xmlResult));
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Copy seg_editor folders
	for (int dy = 0; dy < vSize.y; ++dy)
	{
		for (int dx = 0; dx < vSize.x; ++dx)
		{
			CString destSegDir, srcSegDir;
			destSegDir.Format(SW_SegmentDirPattern,vDestTopLeft.x + dx, vDestTopLeft.y + dy);
			srcSegDir.Format(SW_SegmentDirPattern, vSrcTopLeft.x + dx, vSrcTopLeft.y + dy);
			if (CFileUtil::PathExists(srcEditorSegsRoot + srcSegDir))	// hole(s) are allowed so we have to check each directory
			{
				CFileUtil::CreatePath(destEditorSegsRoot+destSegDir+"/");
				CFileUtil::CopyTree( srcEditorSegsRoot+srcSegDir + "/", destEditorSegsRoot+destSegDir + "/");
			}
			progressBar.Step( 100*nCurStep++/nTotoalSteps );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Copy seg folders
	for (int dy = 0; dy < vSize.y; ++dy)
	{
		for (int dx = 0; dx < vSize.x; ++dx)
		{
			CString destSegDir, srcSegDir;
			destSegDir.Format(SW_SegmentDirPattern,vDestTopLeft.x + dx, vDestTopLeft.y + dy);
			srcSegDir.Format(SW_SegmentDirPattern, vSrcTopLeft.x + dx, vSrcTopLeft.y + dy);
			if (CFileUtil::PathExists(srcSegsRoot + srcSegDir))	// hole(s) are allowed so we have to check each directory
			{
				CFileUtil::CreatePath(destSegsRoot+destSegDir+"/");
				CFileUtil::CopyTree( srcSegsRoot+srcSegDir + "/", destSegsRoot+destSegDir + "/");
			}
			// update offset info for each segment that will be used for global positions in visarea
			CPakFile segPak;
			CString pakFile, offsetFile;
			XmlNodeRef offsetXML;
			pakFile.Format("%s/"SW_SegmentsDirPath SW_SegmentDirPattern"/%s", strDestPath, vDestTopLeft.x + dx, vDestTopLeft.y + dy, SW_SegPakFile);
			offsetFile.Format("%s/"SW_SegmentsDirPath SW_SegmentDirPattern"/offsetinfo.xml", strDestPath, vDestTopLeft.x + dx, vDestTopLeft.y + dy);
			if(gEnv->pCryPak->OpenPack(pakFile))
			{
				Vec2 vOff(0, 0);
				offsetXML = XmlHelpers::LoadXmlFromFile(offsetFile);

				if(offsetXML)
					offsetXML->getAttr("vOff", vOff);
				else
					offsetXML = XmlHelpers::CreateXmlNode("OffsetInfo");

				vOff += vDiffSrcToDest;
				offsetXML->setAttr("vOff", vOff);

				gEnv->pCryPak->ClosePack(pakFile);
			}
			if(segPak.Open(pakFile, true))
			{
				XmlString xmlString = offsetXML->getXML();
				segPak.GetArchive()->UpdateFile(offsetFile,(void *)xmlString.data(), xmlString.size());
				segPak.Close();
			}
			progressBar.Step( 100*nCurStep++/nTotoalSteps );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Merge layerinfo.xml
	typedef std::map<GUID,XmlNodeRef, guid_less_predicate> TGuidXmlNodeMap;
	TGuidXmlNodeMap layermapping;
	XmlNodeRef defaultLayer;
	std::vector<GUID> otherMainLayerGUIDs;
	{
		XmlNodeRef resultLayerInfo = XmlHelpers::CreateXmlNode("Layers");
		XmlNodeRef srcLayerInfos[SourceData_count];
		srcLayerInfos[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(strDestPath + "/layerinfo.xml");
		srcLayerInfos[SourceData_Src] = XmlHelpers::LoadXmlFromFile(strSrcPath + "/layerinfo.xml");

		nTotoalSteps += 0 - nProgStageEstimate + srcLayerInfos[SourceData_Dest]->getChildCount() + srcLayerInfos[SourceData_Src]->getChildCount();

		size_t nLayerIdx = 1;	// layer index starts from 1
		for (size_t iRoot = 0; iRoot < SourceData_count; ++iRoot)
		{
			XmlNodeRef layerinfo = srcLayerInfos[iRoot];
			assert(layerinfo->isTag("Layers"));
			for (size_t ii = 0; ii < layerinfo->getChildCount(); ++ii)
			{
				progressBar.Step( 100*nCurStep++/nTotoalSteps );

				XmlNodeRef nodeLayer = layerinfo->getChild(ii);
				assert(nodeLayer->isTag("Layer"));
				// we ignored layer id here because that will be re sorted

				XmlNodeRef nodeLayerAttr = nodeLayer->getChild(0);
				assert(nodeLayerAttr->isTag("LayerAttribs"));


				GUID guid;
				nodeLayerAttr->getAttr("GUID",guid);
				if (layermapping.find(guid) != layermapping.end())
					continue;					// same layer exists
				
				XmlNodeRef nodeLayerNew = XmlHelpers::CreateXmlNode("Layer");
				nodeLayerNew->setAttr("ID",nLayerIdx++);
				nodeLayerNew->addChild(nodeLayerAttr);

				// handle main layers
				if (0 == stricmp(nodeLayerAttr->getAttr("FullName"), "Main"))	
				{
					if (!defaultLayer)
						defaultLayer = nodeLayerNew;
					else	// even more Main layers
					{
						otherMainLayerGUIDs.push_back(guid);
						continue;							// skip
					}
				}

				resultLayerInfo->addChild(nodeLayerNew);

				// fill layer mapping
				layermapping.insert(std::make_pair(guid,nodeLayerNew));
			}
		}
		assert(resultLayerInfo->getChildCount());	// at least one layer!
		if (!defaultLayer && resultLayerInfo->getChildCount())
			defaultLayer = resultLayerInfo->getChild(0);	// if no main layer pick any layer that comes first.
		XmlHelpers::SaveXmlNode(resultLayerInfo,strDestPath+"/layerinfo.xml");
	}
	assert(otherMainLayerGUIDs.size() <= 1); // there should be only one other Main layer

	//////////////////////////////////////////////////////////////////////////
	// Replace Main.lyr files in dest folder
	assert( arrMainLayerFragments.empty() || 0 == stricmp(defaultLayer->findChild("LayerAttribs")->getAttr("FullName"), "Main") );
	GUID guidDefMainLayer;
	bool bRetval = defaultLayer->findChild("LayerAttribs")->getAttr("GUID",guidDefMainLayer);
	assert(bRetval);
	for (size_t ii = 0; ii < arrMainLayerFragments.size(); ++ii)
	{
		XmlNodeRef xml = arrMainLayerFragments[ii].second;
		for (int jj = 0; jj < xml->getChildCount(); ++jj)
			xml->getChild(jj)->setAttr("LayerGUID", guidDefMainLayer);	// put all objects to default layer

		CString destSegDir;
		CPoint& wc = arrMainLayerFragments[ii].first;
		destSegDir.Format(SW_SegmentDirPattern, wc.x, wc.y);

		CString strFile = destEditorSegsRoot + destSegDir + "/Main.lyr";
		CFileUtil::DeleteFile(strFile);
		XmlHelpers::SaveXmlNode(xml, strFile);
	}

	//////////////////////////////////////////////////////////////////////////
	// Merge segmentinfo.xml
	TRectI rcBoundsDest;
	{
		XmlNodeRef srcSegInfos[SourceData_count];
		srcSegInfos[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(strDestPath + "/segmentinfo.xml");
		srcSegInfos[SourceData_Src] = XmlHelpers::LoadXmlFromFile(strSrcPath + "/segmentinfo.xml");

		// merge world bounds
		XmlNodeRef resultBounds = XmlHelpers::CreateXmlNode("WorldBounds");
		{
			Vec2 vTmpMin,vTmpMax;
			srcSegInfos[SourceData_Dest]->findChild("WorldBounds")->getAttr("Min",vTmpMin);
			srcSegInfos[SourceData_Dest]->findChild("WorldBounds")->getAttr("Max",vTmpMax);
			rcBoundsDest = TRectI(vTmpMin.x,vTmpMin.y,vTmpMax.x,vTmpMax.y);
			rcBoundsDest.DoUnite(rectDest);	// just extend it with the merged area
			resultBounds->setAttr("Min", Vec2(rcBoundsDest.Min.x,rcBoundsDest.Min.y));
			resultBounds->setAttr("Max", Vec2(rcBoundsDest.Max.x,rcBoundsDest.Max.y));
		}

		srcSegInfos[SourceData_Src] = srcSegInfos[SourceData_Src]->findChild("SegLayerMappings");
		srcSegInfos[SourceData_Dest] = srcSegInfos[SourceData_Dest]->findChild("SegLayerMappings");
		assert(srcSegInfos[SourceData_Src] && srcSegInfos[SourceData_Dest]);

		nTotoalSteps += 0 - nProgStageEstimate + srcSegInfos[SourceData_Dest]->getChildCount() + srcSegInfos[SourceData_Src]->getChildCount();

		XmlNodeRef resultSegInfo = XmlHelpers::CreateXmlNode("SegLayerMappings");
		// get those seginfo in range, and fill dest file
		for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
		{
			XmlNodeRef seginfo = srcSegInfos[idxRoot];
			for (size_t ii = 0; ii < seginfo->getChildCount(); ++ii)
			{
				XmlNodeRef nodeSeg = seginfo->getChild(ii);
				assert(nodeSeg->isTag("Segment"));

				GUID guidLayerInSeg;
				nodeSeg->getAttr("LayerGUID",guidLayerInSeg);

				// this is an obsoleted main layer, replace the guid with the new main layer guid
				if ( !otherMainLayerGUIDs.empty() && otherMainLayerGUIDs[0] == guidLayerInSeg)
				{
					guidLayerInSeg = guidDefMainLayer;
					nodeSeg->setAttr("LayerGUID", guidLayerInSeg);
				}

				TGuidXmlNodeMap::const_iterator iterLayer = layermapping.find(guidLayerInSeg);
				if (iterLayer == layermapping.end())
					continue;		// if no such layer in layerinfo, we should not put the mapping in segmentinfo.

				XmlNodeRef layerNode = iterLayer->second;

				if (idxRoot == SourceData_Src)
				{
					// skip not needed and offset positions of those needed
					Vec2 tmpwc;
					nodeSeg->getAttr("wc",tmpwc);
					Vec2i wcSrc((int)tmpwc.x, (int)tmpwc.y);

					if (!rectSrc.InRect(wcSrc))
						continue;															// skip if not in range

					nodeSeg->setAttr("wc", Vec2(vDiffSrcToDest.x + wcSrc.x, vDiffSrcToDest.y + wcSrc.y));
				}


				// Layer id and name should be changed to new id and new name
				nodeSeg->setAttr("LayerID", layerNode->getAttr("ID"));
				nodeSeg->setAttr("LayerName", layerNode->getChild(0)->getAttr("Name"));
				resultSegInfo->addChild(nodeSeg);

				progressBar.Step( 100*nCurStep++/nTotoalSteps );
			}
		}

		XmlNodeRef xmlFileWrapper = XmlHelpers::CreateXmlNode("SegmentInfo");
		xmlFileWrapper->addChild(resultBounds);
		xmlFileWrapper->addChild(resultSegInfo);
		XmlHelpers::SaveXmlNode(xmlFileWrapper, strDestPath + "/segmentinfo.xml");
	}

	//////////////////////////////////////////////////////////////////////////
	// merge objectinfo.xml (external objects)
	typedef std::deque<GUID> TGuidList;
	TGuidList lstGuidNeedToCopy;
	{
		TGuidSet	guidsetObjs;		// external objects may cross several segments, so merging by segs may cause duplicated objects.
		XmlNodeRef resultObjInfo = XmlHelpers::CreateXmlNode("Objects");
		XmlNodeRef srcObjInfos[SourceData_count];
		srcObjInfos[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(strDestPath + "/objectinfo.xml");
		srcObjInfos[SourceData_Src] = XmlHelpers::LoadXmlFromFile(strSrcPath + "/objectinfo.xml");

		nTotoalSteps += 0 - nProgStageEstimate + srcObjInfos[SourceData_Dest]->getChildCount() + srcObjInfos[SourceData_Src]->getChildCount();

		for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
		{
			XmlNodeRef objinfo = srcObjInfos[idxRoot];
			assert(objinfo->isTag("Objects"));
			for(size_t ii = 0; ii < objinfo->getChildCount(); ++ii)
			{
				XmlNodeRef nodeObj = objinfo->getChild(ii);
				assert(nodeObj->isTag("Object"));

				GUID guidObj;
				nodeObj->getAttr("GUID", guidObj);
				if ( !guidsetObjs.insert(guidObj).second )	// we already have this object
					continue;

				GUID guidLayer;
				nodeObj->getAttr("LayerGUID", guidLayer);

				XmlNodeRef layerNode = layermapping[guidLayer];
				if (!layerNode)		
					layerNode = defaultLayer;			// if no such layer in layermap, use default layer
				if (!layerNode)
					continue;							// if still no, leave it

				if (idxRoot == SourceData_Src)
				{
					// skip not needed and offset positions of those needed
					TRectI rectObj;
					Vec2 tmpwc;
					nodeObj->getAttr("wcBoxMin", tmpwc);
					rectObj.Min.set((int)tmpwc.x, (int)tmpwc.y);
					nodeObj->getAttr("wcBoxMax", tmpwc);
					rectObj.Max.set((int)tmpwc.x, (int)tmpwc.y);

					nodeObj->getAttr("wc", tmpwc);
					Vec2i mainwc((int)tmpwc.x, (int)tmpwc.y);

					if (!rectObj.Intersects(rectSrc) && !rectSrc.InRect(mainwc))
						continue;															// skip if not in range

					nodeObj->setAttr("wc", Vec2(vDiffSrcToDest.x + mainwc.x, vDiffSrcToDest.y + mainwc.y));

					nodeObj->setAttr("wcBoxMin", Vec2(vDiffSrcToDest.x + rectObj.Min.x, vDiffSrcToDest.y + rectObj.Min.y) );
					nodeObj->setAttr("wcBoxMax", Vec2(vDiffSrcToDest.x + rectObj.Max.x, vDiffSrcToDest.y + rectObj.Max.y) );

					lstGuidNeedToCopy.push_back(guidObj);
				}

				nodeObj->setAttr("LayerID", layerNode->getAttr("ID"));
				nodeObj->setAttr("LayerName", layerNode->getChild(0)->getAttr("Name"));

				resultObjInfo->addChild(nodeObj);

				progressBar.Step( 100*nCurStep++/nTotoalSteps );
			}
		}
		XmlHelpers::SaveXmlNode(resultObjInfo, strDestPath + "/objectinfo.xml");
	}

	//////////////////////////////////////////////////////////////////////////
	// copy external object files
	{
		nTotoalSteps += 0 - nProgStageEstimate + lstGuidNeedToCopy.size();

		for (TGuidList::const_iterator iter = lstGuidNeedToCopy.begin(); iter != lstGuidNeedToCopy.end(); ++iter)
		{
			CString strSrcObjFile = strSrcPath + "/"SW_ExternalObjectDirName"/" + GuidUtil::ToString(*iter);
			CString strDestObjFile = strDestPath + "/"SW_ExternalObjectDirName"/" + GuidUtil::ToString(*iter);
			CFileUtil::CopyFile( strSrcObjFile, strDestObjFile, true);

			progressBar.Step( 100*nCurStep++/nTotoalSteps );
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// merge global terrain layers data
	{
		XmlNodeRef xmlResultST = XmlHelpers::CreateXmlNode("SurfaceTypes");
		XmlNodeRef xmlResultTL = XmlHelpers::CreateXmlNode("Layers");
		XmlNodeRef xmlSourcesRoot[SourceData_count];
		xmlSourcesRoot[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(strDestPath + "/" + g_szWDBNodeFileNames[WDB_TERRAIN_LAYERS]);
		xmlSourcesRoot[SourceData_Src] = XmlHelpers::LoadXmlFromFile(strSrcPath + "/" + g_szWDBNodeFileNames[WDB_TERRAIN_LAYERS]);
		assert(xmlSourcesRoot[SourceData_Src]->isTag(g_pcWDBNodeNames[WDB_TERRAIN_LAYERS][WDBNT_ROOT]));
		assert(xmlSourcesRoot[SourceData_Dest]->isTag(g_pcWDBNodeNames[WDB_TERRAIN_LAYERS][WDBNT_ROOT]));

		// Process Surface Types
		{
			typedef std::set<string> TStringSet;
			TStringSet setSurfaceTypeName;

			XmlNodeRef xmlSources[SourceData_count];
			xmlSources[SourceData_Dest] = xmlSourcesRoot[SourceData_Dest]->findChild("SurfaceTypes");
			xmlSources[SourceData_Src] = xmlSourcesRoot[SourceData_Src]->findChild("SurfaceTypes");

			nTotoalSteps += 0 - nProgStageEstimate + xmlSources[SourceData_Dest]->getChildCount() + xmlSources[SourceData_Src]->getChildCount();

			uint32 idDataNew = 0;
			for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
			{
				XmlNodeRef xmlDataset = xmlSources[idxRoot];
				for(size_t ii = 0; ii < xmlDataset->getChildCount(); ++ii)
				{
					XmlNodeRef xmlData = xmlDataset->getChild(ii);
					assert(xmlData->isTag("SurfaceType"));

					CString strKey;
					xmlData->getAttr("Name",strKey);

					if (!setSurfaceTypeName.insert(string((const char*)strKey)).second)
						continue;

					xmlData->setAttr("SurfaceTypeID", idDataNew++);
					xmlResultST->addChild(xmlData);

					progressBar.Step( 100*nCurStep++/nTotoalSteps );
				}
			}
		}

		// Process Terrain Layers
		{
			typedef uint32 TBitfield[256/32];				// 255 is the biggest layerId that we can have
			TGuidSet setGuids;
			TBitfield bitsLayerId = {};

			XmlNodeRef xmlSources[SourceData_count];
			xmlSources[SourceData_Dest] = xmlSourcesRoot[SourceData_Dest]->findChild("Layers");
			xmlSources[SourceData_Src] = xmlSourcesRoot[SourceData_Src]->findChild("Layers");

			nTotoalSteps += 0 - nProgStageEstimate + xmlSources[SourceData_Dest]->getChildCount() + xmlSources[SourceData_Src]->getChildCount();

			uint32 idDataNew = 0;
			for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
			{
				XmlNodeRef xmlDataset = xmlSources[idxRoot];
				for(size_t ii = 0; ii < xmlDataset->getChildCount(); ++ii)
				{
					XmlNodeRef xmlData = xmlDataset->getChild(ii);
					assert(xmlData->isTag("Layer"));

					GUID guidKey;
					xmlData->getAttr("GUID",guidKey);
					if (!setGuids.insert(guidKey).second)
						continue;

					uint32 idLayer;
					xmlData->getAttr("LayerId", idLayer);
					if (bitsLayerId[idLayer])		// layerId already taken, find a new one
					{
						uint32 idLayerNew;
						size_t jj = 0;
						for (; jj < _countof(bitsLayerId); ++jj )
						{
							uint32 input = bitsLayerId[jj];
							if (input == 0xFFffFFff)
								continue;

							uint32 idx = 0;
							while (input & (1<<idx));
							idLayerNew = jj*32 + idx;
							break;
						}
						if (jj >= _countof(bitsLayerId))
						{
							SWLog("Total terrain layer exceeded the maximum 256!");
							break;
						}
						xmlData->setAttr("LayerId", idLayerNew);
					}
					xmlResultTL->addChild(xmlData);

					progressBar.Step( 100*nCurStep++/nTotoalSteps );
				}
			}
		}

		XmlNodeRef resultFileWrapper = XmlHelpers::CreateXmlNode(g_pcWDBNodeNames[WDB_TERRAIN_LAYERS][WDBNT_ROOT]);
		resultFileWrapper->addChild(xmlResultST);
		resultFileWrapper->addChild(xmlResultTL);
		XmlHelpers::SaveXmlNode(resultFileWrapper, strDestPath + "/" + g_szWDBNodeFileNames[WDB_TERRAIN_LAYERS]);
	}

	//////////////////////////////////////////////////////////////////////////
	{
		TGuidSet	guidsetObjs;
		// merge global vegetation data
		{
			XmlNodeRef xmlResult = XmlHelpers::CreateXmlNode("Objects");
			XmlNodeRef xmlSources[SourceData_count];
			xmlSources[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(strDestPath + "/" + g_szWDBNodeFileNames[WDB_VEGETATION]);
			xmlSources[SourceData_Src] = XmlHelpers::LoadXmlFromFile(strSrcPath + "/" + g_szWDBNodeFileNames[WDB_VEGETATION]);
			xmlSources[SourceData_Dest] = xmlSources[SourceData_Dest]->getChild(0)->getChild(0);
			xmlSources[SourceData_Src] = xmlSources[SourceData_Src]->getChild(0)->getChild(0);

			nTotoalSteps += 0 - nProgStageEstimate + xmlSources[SourceData_Dest]->getChildCount() + xmlSources[SourceData_Src]->getChildCount();

			uint32 idDataNew = 0;
			for (size_t idxRoot = 0; idxRoot < SourceData_count; ++idxRoot)
			{
				XmlNodeRef xmlDataset = xmlSources[idxRoot];
				assert(xmlDataset->isTag("Objects"));
				for(size_t ii = 0; ii < xmlDataset->getChildCount(); ++ii)
				{
					XmlNodeRef xmlData = xmlDataset->getChild(ii);
					assert(xmlData->isTag("Object"));

					GUID guidData;
					xmlData->getAttr("GUID",guidData);

					if (!guidsetObjs.insert(guidData).second) // already have same guid
						continue;

					// regenerate id
					xmlData->setAttr("Id",idDataNew++);
					xmlResult->addChild(xmlData);

					progressBar.Step( 100*nCurStep++/nTotoalSteps );
				}
			}

			XmlNodeRef resultFileWrapper = XmlHelpers::CreateXmlNode(g_pcWDBNodeNames[WDB_VEGETATION][WDBNT_ROOT]);
			resultFileWrapper->newChild("VegetationMap")->addChild(xmlResult);
			XmlHelpers::SaveXmlNode(resultFileWrapper, strDestPath + "/" + g_szWDBNodeFileNames[WDB_VEGETATION]);
		}

		// remap vegetation object id in every instance
		{
			// the id is never used for loading, and instances are mapped to VegObject by GUID, so we don't need to remap at all.
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Merge level pak
	{
		XmlNodeRef xmlResultMission = NULL, xmlResultLevelData = NULL, xmlResultLevelInfo = NULL;
		CString srcMissionFile, destMissionFile, srcLevelDataFile, destLevelDataFile, destLevelInfoFile;
		srcMissionFile.Format("%s/mission_mission0.xml", strSrcPath);
		destMissionFile.Format("%s/mission_mission0.xml", strDestPath);
		srcLevelDataFile.Format("%s/leveldata.xml", strSrcPath);
		destLevelDataFile.Format("%s/leveldata.xml", strDestPath);
		destLevelInfoFile.Format("%s/levelinfo.xml", strDestPath);

		if(gEnv->pCryPak->OpenPack(strSrcPackFile) && gEnv->pCryPak->OpenPack(strDestPackFile))
		{
			// process global entities
			{
				XmlNodeRef xmlSrcMission = XmlHelpers::LoadXmlFromFile(srcMissionFile);
				xmlResultMission = XmlHelpers::LoadXmlFromFile(destMissionFile);

				if(xmlSrcMission && xmlResultMission)
				{
					XmlNodeRef srcObjRoot = xmlSrcMission->findChild("Objects");
					XmlNodeRef destObjRoot = xmlResultMission->findChild("Objects");
					if(srcObjRoot && destObjRoot)
					{
						for(int i = 0; i < srcObjRoot->getChildCount(); i++)
						{
							XmlNodeRef obj = srcObjRoot->getChild(i);

							Vec2 tmpwc;
							obj->getAttr("CoordInSW",tmpwc);
							Vec2i wcSrc((int)tmpwc.x, (int)tmpwc.y);

							if (!rectSrc.InRect(wcSrc))
								continue;

							tmpwc += vDiffSrcToDest;
							obj->setAttr("CoordInSW",tmpwc);

							destObjRoot->addChild(obj);
						}
					}
				}
			}

			// process surface types 
			{
				std::set<string> surfaceTypeNames;

				XmlNodeRef xmlSourcesRoot[SourceData_count];
				xmlSourcesRoot[SourceData_Src] = XmlHelpers::LoadXmlFromFile(srcLevelDataFile);
				xmlSourcesRoot[SourceData_Dest] = XmlHelpers::LoadXmlFromFile(destLevelDataFile);
				xmlResultLevelData = XmlHelpers::LoadXmlFromFile(destLevelDataFile);

				if(xmlSourcesRoot[SourceData_Src] && xmlSourcesRoot[SourceData_Dest])
				{
					XmlNodeRef xmlSources[SourceData_count];
					xmlSources[SourceData_Src] = xmlSourcesRoot[SourceData_Src]->findChild("SurfaceTypes");
					xmlSources[SourceData_Dest] = xmlSourcesRoot[SourceData_Dest]->findChild("SurfaceTypes");
				
					uint32 idDataNew = 0;
					if(xmlSources[SourceData_Src] && xmlSources[SourceData_Dest])
					{
						XmlNodeRef xmlResultST = XmlHelpers::CreateXmlNode("SurfaceTypes");
						for(int j = 0; j < SourceData_count; j++)
						{
							XmlNodeRef xmlDataset = xmlSources[j];
							for(int i = 0; i < xmlDataset->getChildCount(); i++)
							{
								XmlNodeRef xmlData = xmlDataset->getChild(i);
								assert(xmlData->isTag("SurfaceType"));

								CString stName;
								xmlData->getAttr("Name", stName);
							
								if (!surfaceTypeNames.insert(string((const char*)stName)).second)
									continue;

								xmlData->setAttr("SurfaceTypeID", idDataNew++);
								xmlResultST->addChild(xmlData);
							}
						}
						xmlResultLevelData->removeChild(xmlResultLevelData->findChild("SurfaceTypes"));
						xmlResultLevelData->addChild(xmlResultST);
					}
				}
			}

			// process world bounding box
			{
				xmlResultLevelInfo = XmlHelpers::LoadXmlFromFile(destLevelInfoFile);
				if(xmlResultLevelInfo)
				{
					xmlResultLevelInfo->setAttr( "SegmentedWorldMinX", rcBoundsDest.Min.x );
					xmlResultLevelInfo->setAttr( "SegmentedWorldMinY", rcBoundsDest.Min.y );
					xmlResultLevelInfo->setAttr( "SegmentedWorldMaxX", rcBoundsDest.Max.x );
					xmlResultLevelInfo->setAttr( "SegmentedWorldMaxY", rcBoundsDest.Max.y );
				}
			}

			gEnv->pCryPak->ClosePack(strDestPackFile);
			gEnv->pCryPak->ClosePack(strSrcPackFile);
		}

		if(destPak.Open(strDestPackFile, true))
		{
			XmlString xmlString;
			if(xmlResultMission)
			{
				xmlString = xmlResultMission->getXML();
				destPak.GetArchive()->UpdateFile(destMissionFile, (void *)xmlString.data(), xmlString.size());
			}
			if(xmlResultLevelData)
			{
				xmlString = xmlResultLevelData->getXML();
				destPak.GetArchive()->UpdateFile(destLevelDataFile, (void *)xmlString.data(), xmlString.size());
			}
			if(xmlResultLevelInfo)
			{
				xmlString = xmlResultLevelInfo->getXML();
				destPak.GetArchive()->UpdateFile(destLevelInfoFile, (void *)xmlString.data(), xmlString.size());
			}
			destPak.Close();
		}
	}

	progressBar.Stop();

	//////////////////////////////////////////////////////////////////////////
	// TODO: do we need a report??


	return true;
}

bool CSegmentedWorldManager::RemoveSegments( CRect const& rcArea, const char* szWorldName /*= NULL*/ )
{
	std::vector<CPoint> wcs;
	for (int dy = 0; dy < rcArea.Width(); ++dy)
		for (int dx = 0; dx < rcArea.Height(); ++dx)
			wcs.push_back(rcArea.TopLeft()+CSize(dx,dy));

	return RemoveSegments(wcs, szWorldName);
}

bool CSegmentedWorldManager::RemoveSegments( std::vector<CPoint> const& wcs, const char* szWorldName /*= NULL*/ )
{
	if (!szWorldName && IsSWDoc())
		szWorldName = GetDoc().GetWorldName();
	if (!szWorldName)
		return false;

	CString strLevelPath = CString("Levels/") + szWorldName;
	strLevelPath = Path::GamePathToFullPath(strLevelPath);
	Path::ConvertBackSlashToSlash(strLevelPath);

	if (!CFileUtil::PathExists(strLevelPath))
	{
		SWLog("Level not exists!");
		return false;
	}

	if (GetIEditor()->GetDocument()->IsDocumentReady() && IsSWDoc()) // if a SW level is opened check if it conflicts with this operation
	{
		CString strCurLevel = Path::GamePathToFullPath(GetIEditor()->GetLevelFolder());
		Path::ConvertBackSlashToSlash(strCurLevel);

		if (0==strLevelPath.CompareNoCase(strCurLevel))
		{
			SWLog("You cannot remove segments from a level which are currently opened for editing");
			return false;
		}
	}

	// Check data structure version
	if (CDataUpgrade::Get().DetectSWLevelVersion(CreateTempPersistence(strLevelPath)) != CSWManager::GetSWDataStructureVersion())
	{
		SWMsgBox("The level is not in new format, please upgrade it first");
		return false;
	}

	for	(std::vector<CPoint>::const_iterator iter = wcs.begin(); iter != wcs.end(); ++iter )
	{
		const CPoint& wc = *iter;
		// remove seg_editor folder
		{
			CString strSegPath;
			strSegPath.Format("%s/" SW_SegmentsDirPath SW_SegmentDirPattern "/", strLevelPath, wc.x, wc.y);
			Path::ConvertBackSlashToSlash(strSegPath);
			CFileUtil::Deltree(strSegPath, true);
		}

		// remove seg folder
		{
			CString strSegPath;
			strSegPath.Format("%s/" SW_EditorDataDirPath SW_SegmentDirPattern "/", strLevelPath, wc.x, wc.y);
			Path::ConvertBackSlashToSlash(strSegPath);
			CFileUtil::Deltree(strSegPath, true);
		}
	}

	std::set<GUID, guid_less_predicate> remainingLayers;
	// update segmentinfo.xml
	{
		CString strFilename = strLevelPath + "/segmentinfo.xml";
		XmlNodeRef xmlSegInfo;
		xmlSegInfo = XmlHelpers::LoadXmlFromFile(strFilename);


		XmlNodeRef seglayers = xmlSegInfo->findChild("SegLayerMappings");
		for (size_t ii = 0; ii < seglayers->getChildCount(); ++ii)
		{
			XmlNodeRef nodeSeg = seglayers->getChild(ii);
			assert(nodeSeg->isTag("Segment"));

			Vec2 curwc;
			nodeSeg->getAttr("wc",curwc);
			if (std::find(wcs.begin(), wcs.end(), CPoint(curwc.x,curwc.y)) != wcs.end())
			{
				// remove segments
				seglayers->deleteChildAt(ii--);
			}
			else
			{
				// update remainings & recalculate world bounds
				GUID guidLayer;
				nodeSeg->getAttr("LayerGUID",guidLayer);
				remainingLayers.insert(guidLayer);			// meant to have many equal values

				Vec2 curwc;
				nodeSeg->getAttr("wc",curwc);


			}
		}

		enum{ RECT_INVALID_VALUE = 0x7fFFffFF };
		static const CRect _invalid_rect(RECT_INVALID_VALUE,RECT_INVALID_VALUE,RECT_INVALID_VALUE,RECT_INVALID_VALUE);
		CRect worldBounds = _invalid_rect;

			CString strSegs;
			strSegs.Format("%s/" SW_EditorDataDirPath "*.*" , strLevelPath);
			Path::ConvertBackSlashToSlash(strSegs);

		__finddata64_t		fd;
		intptr_t hFind = 0;
		if ((hFind = _findfirst64(strSegs, &fd)) != -1)
		{
			do
			{
				if (!(fd.attrib & _A_SUBDIR))
					continue;

				CString name = fd.name;

				int x,y;
				if (2 != sscanf(name, SW_SegmentDirPattern, &x, &y))
					continue;

				CRect rcCur = CRect(CPoint(x,y),CSize(1,1));

				// if not initialized give it a initial value
				if ( worldBounds == _invalid_rect )
					worldBounds = rcCur;
				else
					worldBounds.UnionRect(worldBounds, rcCur);

			} while(!_findnext64(hFind, &fd));

			_findclose(hFind);
		}

		if (worldBounds != _invalid_rect)
		{
			xmlSegInfo->findChild("WorldBounds")->setAttr("Min", Vec2(worldBounds.left,worldBounds.top));
			xmlSegInfo->findChild("WorldBounds")->setAttr("Max", Vec2(worldBounds.right-1,worldBounds.bottom-1));
		}

		CFileUtil::DeleteFile(strFilename);
		XmlHelpers::SaveXmlNode(xmlSegInfo, strFilename);
	}

	// update layerinfo.xml 
	{
		CString strFilename = strLevelPath + "/layerinfo.xml";

		XmlNodeRef xmlLayerInfo;
		xmlLayerInfo = XmlHelpers::LoadXmlFromFile(strFilename);
		assert(xmlLayerInfo->isTag("Layers"));

		for (size_t ii=0; ii < xmlLayerInfo->getChildCount(); ++ii)
		{
			XmlNodeRef nodeLayer = xmlLayerInfo->getChild(ii)->getChild(0);
			if (!nodeLayer)
				continue;
			assert(nodeLayer->isTag("LayerAttribs"));
			GUID guidLayer;
			nodeLayer->getAttr("GUID",guidLayer);

			// remove layers that are no longer needed
			if (remainingLayers.end() == remainingLayers.find(guidLayer))
				xmlLayerInfo->deleteChildAt(ii--);
		}

		CFileUtil::DeleteFile(strFilename);
		XmlHelpers::SaveXmlNode(xmlLayerInfo, strFilename);
	}
	// update objectinfo.xml and delete external objects ??
	// Update levelinfo.xml ??
	return true;
}

void CSegmentedWorldManager::GetMissingSegmentsInRect(const char *levelpath, const CRect &rect, std::list<CRect> &segRect)
{
	CString strSegs;
	for(int j = rect.top; j < rect.bottom; j++)
	{
		for(int i = rect.left; i < rect.right; i++)
		{
			strSegs.Format("%s\\" SW_EditorDataDirPath "seg%d_%d" , levelpath, i, j);
			Path::ConvertBackSlashToSlash(strSegs);
			if(!CFileUtil::PathExists(strSegs))
				segRect.push_back(CRect(i, j, i+1, j+1));
		}
	}
}

bool CSegmentedWorldManager::GetWorldBounds(const char *levelpath, CRect &rect)
{
	CString sXML = levelpath + CString("segmentinfo.xml");
	if(!CFileUtil::FileExists(sXML))
		return false;

	XmlNodeRef xml = XmlHelpers::LoadXmlFromFile(sXML);
	if(!xml)
		return false;

	XmlNodeRef xmlWorldBounds = xml->findChild("WorldBounds");
	if(!xmlWorldBounds)
		return false;

	Vec2 tmpMin,tmpMax;
	xmlWorldBounds->getAttr("Min",tmpMin);
	xmlWorldBounds->getAttr("Max",tmpMax);
	rect = CRect(tmpMin.x, tmpMin.y, tmpMax.x, tmpMax.y);

	return true;
}

void CSegmentedWorldManager::OnSWDrawBoundingBoxVarChange( ICVar *pArgs )
{
	int val = pArgs->GetIVal();
	CSWManager::Get().SetDebugDraw(val);
}

void CSegmentedWorldManager::RegisterConsoleVariables()
{
	// segmented world
	doc_enable_segmented_world = gEnv->pConsole->GetCVar("doc_enable_segmented_world");
	if (!doc_enable_segmented_world)
	{
		doc_enable_segmented_world = REGISTER_INT("doc_enable_segmented_world",0,0,"Enable saving of segmented maps.\nDefault is 0.\n");
	}

	doc_segmented_world_mode = gEnv->pConsole->GetCVar("doc_segmented_world_mode");
	if (!doc_segmented_world_mode)
	{
		doc_segmented_world_mode = REGISTER_INT("doc_segmented_world_mode",0,0,"Enable saving of segmented maps.\nDefault is 0.\n");
		if (doc_segmented_world_mode)
			doc_segmented_world_mode->SetOnChangeCallback(&CSWManager::OnModeChange);
	}

	REGISTER_INT("sw_draw_bbox",1,0,"Draw bounding box for segments.\nDefault is 1.\n")->SetOnChangeCallback(OnSWDrawBoundingBoxVarChange);
}

// WARNING: This function is used in DataUpgrade, don't forget to "Copy-on-change" to DataUpgrade module.
bool CSegmentedWorldManager::IsSWLevel( const char * szPathName )
{
	CString sPathName, sPath, sFilename, sFileExt;
	sPathName = szPathName;
	Path::Split(sPathName, sPath, sFilename, sFileExt);

	if ( 0 == sFileExt.CompareNoCase(".cry") )
		return false;

	if (CFileUtil::PathExists( Path::RemoveBackslash(sPath)) && CFileUtil::FileExists(sPath + "sw.xml"))
		return true;


	assert(0);
	SWLog("Unidentifiable level files, possible data corruption");
	return false;
}

bool CSegmentedWorldManager::SaveAsSWLevel( CCryEditDoc* pDoc, const char* szFullName )
{
	CCryEditDoc::TSaveDocContext context;
	pDoc->BeforeSaveDocument(szFullName,context);
	{
		//bool& bSaved = context.bSaved;
		//{
			ISWDoc* pSWDoc = GetIEditor()->GetSegmentedWorldManager()->CreateNewDoc(szFullName);
			assert(pSWDoc);
			if (!pSWDoc)
				return FALSE;

			//((CCryEditDoc*)pDoc)->m_pSWDoc = pSWDoc;
		//}

		// Save to Pak file.

		// skip
		//// filename && "TerrainTexture.pak"
		//bSaved = CFileUtil::OverwriteFile( filename ) && CTerrainManager::GetTerrainManager().WouldHeightmapSaveSucceed();

		//if (bSaved)
		{
			// Save Tag Point locations to file if auto save of tag points disabled
			if (!gSettings.bAutoSaveTagPoints)
				((CCryEditApp *)(AfxGetApp()))->SaveTagLocations();

			// Changes filename for this document.
			pDoc->SetPathName( szFullName );

			/*bSaved = */pSWDoc->Save(false);
		}
	}
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

int CSegmentedWorldManager::GetSegmentSizeMeters()
{
	return SEGMENT_SIZE_METERS;
}

TSWDataStructureVersion CSegmentedWorldManager::MayUpgradeLevel( sw::ISWDataPersistence* pPersist )
{
	TSWDataStructureVersion verLevel = CDataUpgrade::Get().DetectSWLevelVersion(pPersist);
	if (verLevel == SWDStructVer_Invalid)
		return SWDStructVer_Invalid;

	SWLog("SWDataStructureVersion: %d",verLevel);

	if (verLevel < CSWManager::GetSWDataStructureVersion())
	{
		SWLog("Upgrading Level from version(%d) to version(%d)",verLevel, CSWManager::GetSWDataStructureVersion());
		CDataUpgrade::EUpgradeResult upgradeResult = CDataUpgrade::Get().Upgrade(&verLevel,pPersist);

		switch (upgradeResult)
		{
		case CDataUpgrade::UR_FAILED:
			SWLog("Failed to upgrade level, stopped at version(%d)",verLevel);
			break;
		case CDataUpgrade::UR_BROKEN:
			SWLog("Failed to upgrade level, stopped at version(%d), and the level might be BROKEN", verLevel);
			break;
		}

		assert(verLevel == CSWManager::GetSWDataStructureVersion());
	}
	return verLevel;
}

sw::TPersistencePtr CSegmentedWorldManager::CreateTempPersistence( const char* pszLevelPath, bool bUseNullVC /*= false*/ )
{
	ISWVersionControl* pVCRaw;
	{
		int bIsOffline = 1;
		CEngineSettingsManager settingsMgr;
		settingsMgr.GetModuleSpecificIntEntry("EDT_SourceControl_Offline", bIsOffline);
		if (! CSWManager::Get().IsMapConverting())
		{
			// Check if the opening level is managed
			CString strLevelFileName = pszLevelPath;
			strLevelFileName += g_szWDBNodeFileNames[WDB_LEVELGENERAL];		// TODO: use WorldData::GetFileName instead
			uint32 nFileAttr = GetIEditor()->GetSourceControl()->GetFileAttributes(strLevelFileName );
			bIsOffline |= ((nFileAttr & SCC_FILE_ATTRIBUTE_MANAGED) == 0);
		}

		if (bIsOffline )
			pVCRaw = new CNullVersionControl();
		else
			pVCRaw = new CNativeSCMAdapter(&CSWManager::Get(),GetIEditor()->GetSourceControl());
	}

	_smart_ptr<sw::ISWVersionControl> pVC = pVCRaw;
	return new CSCDataPersistence(pVC,pszLevelPath);
}

//////////////////////////////////////////////////////////////////////////
// struct CDeferredCommandQueue

void CDeferredCommandQueue::ExecOne()
{
	if (m_cmdQueue.empty())
		return;

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
	if (CSWManager::Get().GetDoc().IsNull() || CSWManager::Get().GetDoc().GetWorldName() != cmd.wname.Name || CSWManager::Get().GetDocI()->GetLoadedRect() != cmd.rcArea)
	{
		// open the level 
		bool bOpenOK = true;
		CTimeValue timeSleep;
		if (CSWManager::Get().GetDoc().IsNull() || cmd.wname.Name != CSWManager::Get().GetDoc().GetWorldName())
		{
			if ( OpenResult_NoError != CSWManager::Get().OpenWorld( cmd.wname, true, &cmd.rcArea) )
				bOpenOK = false;

			// sleep a few seconds to let the level fully loaded
			timeSleep = 5.f;
		}

		// open area
		if (bOpenOK && CSWManager::Get().GetDocI()->GetLoadedRect() != cmd.rcArea)
		{
			if (!CSWManager::Get().GetDoc().MoveTo(cmd.rcArea,true))
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
