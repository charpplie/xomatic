/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2013.
*************************************************************************/

#include "stdafx.h"
#include "LODGeneratorDialog.h"
#include "MaterialLODGeneratorDialog.h"
#include "LODInterface.h"
#include "LODGeneratorSharedUI.h"
#include "Material/Material.h"
#include "ViewPaneManager.h"
#include "PropertiesPanel.h"

//////////////////////////////////////////////////////////////////////////
// Main dialog
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CGeometryLodGeneratorDialog, CBaseFrameWnd)

CGeometryLodGeneratorDialog::CGeometryLodGeneratorDialog()
{
	m_pFilePanel = NULL;
	m_pOptionsPanel = NULL;
	m_pTaskPanel = NULL;
	m_pGeoGenPanel = NULL;
	m_pMatOptionsPanel = NULL;
	ClearLodPanels();
	
	CRect rc(0,0,0,0);
	Create( WS_CHILD|WS_VISIBLE,rc,AfxGetMainWnd() );
}

CGeometryLodGeneratorDialog::~CGeometryLodGeneratorDialog()
{
	ClearLodPanels();
	
	m_pFilePanel = NULL;
	m_pOptionsPanel = NULL;
	m_pTaskPanel = NULL;
	m_pGeoGenPanel = NULL;
	m_pMatOptionsPanel = NULL;

	CLodGeneratorInteractionManager::DestroyInstance();
}

void CGeometryLodGeneratorDialog::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CGeometryLodGeneratorDialogViewClass );
	GetIEditor()->GetSettingsManager()->AddToolVersion(LOD_GENERATOR_NAME, LOD_GENERATOR_VER);
	GetIEditor()->GetSettingsManager()->AddToolName(LOD_GENERATOR_LAYOUT_SECTION,"LOD Generator");
}

BEGIN_MESSAGE_MAP(CGeometryLodGeneratorDialog, CBaseFrameWnd)
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_REGISTERED_MESSAGE(WM_GEOM_LOD_FILE_OPENED, OnFileOpened)
	ON_REGISTERED_MESSAGE(WM_GEOM_LOD_CHAIN_GENERATE, OnGenerateLodChain)
	ON_REGISTERED_MESSAGE(WM_GEOM_LOD_CHAIN_CANCEL, OnCancel)
	ON_REGISTERED_MESSAGE(WM_GEOM_LOD_CHAIN_GENERATION_FINISHED, OnLodChainGenerationFinished)
	ON_REGISTERED_MESSAGE(WM_GEOM_LOD_GENERATE_LODS, OnGenerateLods)
	ON_REGISTERED_MESSAGE(WM_GEOM_LOD_REMOVED, OnLodRemoved)
	ON_REGISTERED_MESSAGE(WM_MAT_LOD_GENERATE, OnGenerateMaterial)
	ON_REGISTERED_MESSAGE(WM_MAT_LOD_TEXTURESIZE_CHANGED, OnTextureSizeChanged)
END_MESSAGE_MAP()

BOOL CGeometryLodGeneratorDialog::OnInitDialog()
{
	__super::OnInitDialog();

	CRect rc;
	GetClientRect(rc);

	m_oRollupControl.Create( WS_CHILD|WS_VISIBLE,rc,this,2);
	m_pFilePanel = new CLodGeneratorFilePanel(&m_oRollupControl);
	m_pOptionsPanel = new CGeometryLodGeneratorOptionsPanel(&m_oRollupControl);
	m_pTaskPanel = new CGeometryLodGeneratorTaskPanel(&m_oRollupControl);
	m_pGeoGenPanel = new CGeometryLodGeneratorPreviewPanel(&m_oRollupControl);
	m_pMatOptionsPanel = new CMaterialLODGeneratorOptionsPanel(&m_oRollupControl);
	m_pMatTaskPanel = new CMaterialLODGeneratorTaskPanel(&m_oRollupControl);
	
	m_oRollupControl.InsertPage(CLodGeneratorFilePanel::kPanelCaption, m_pFilePanel);
	m_oRollupControl.InsertPage(CGeometryLodGeneratorOptionsPanel::kPanelCaption, m_pOptionsPanel);
	m_oRollupControl.InsertPage(CGeometryLodGeneratorTaskPanel::kPanelCaption, m_pTaskPanel);
	m_oRollupControl.InsertPage(CGeometryLodGeneratorPreviewPanel::kPanelCaption, m_pGeoGenPanel);
	m_oRollupControl.InsertPage(CMaterialLODGeneratorOptionsPanel::kPanelCaption,m_pMatOptionsPanel);
	m_oRollupControl.InsertPage(CMaterialLODGeneratorTaskPanel::kPanelCaption,m_pMatTaskPanel);
	
	m_pFilePanel->OnBnClickedSelected();

	CVarBlock* pVarBlock = CLodGeneratorInteractionManager::Instance()->GetGeometryVarBlock();
	pVarBlock->AddOnSetCallback(functor(*this, &CGeometryLodGeneratorDialog::OnGeometryVarBlockChanged));

	return TRUE;
}

void CGeometryLodGeneratorDialog::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize( nType, cx, cy );

	if ( m_oRollupControl.GetSafeHwnd() != NULL )
	{
		CRect rc;
		GetClientRect(rc);
		m_oRollupControl.MoveWindow(rc, FALSE);
		m_oRollupControl.Invalidate();
	}
}

void CGeometryLodGeneratorDialog::OnSetFocus(CWnd* pOldWnd)
{
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
	
	CString filepath(pInstance->GetParameterFilePath());

	if(!filepath.IsEmpty())
		m_pFilePanel->OnOpenWithPathParameter(filepath);
}

LRESULT CGeometryLodGeneratorDialog::OnFileOpened(WPARAM wParam, LPARAM lParam)
{
	if (!m_pFilePanel)
		return FALSE;

	if (!m_pOptionsPanel)
		return FALSE;
	
	Reset(true);
	
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
	CString filepath(m_pFilePanel->LoadedFile());
	if(!pInstance->LoadStatObj(filepath))
	{
		CryMessageBox("Failed to load file into the engine, please consult the editor.log","File Load Failed",0);
		return FALSE;
	}

	m_pFilePanel->RefreshMaterialFile();

	CString materialpath(m_pFilePanel->MaterialFile());
	if(!pInstance->LoadMaterial(materialpath))
	{
		CryMessageBox("Failed to load material file into the engine, please consult the editor.log","File Load Failed",0);
		return FALSE;
	}

	m_pGeoGenPanel->CreateExistingLodKeys();
	OnMaterialGeneratePrepare();
	m_pOptionsPanel->Update();

	return TRUE;
}

LRESULT CGeometryLodGeneratorDialog::OnGenerateLodChain(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor wait;
	if ( CLodGeneratorInteractionManager::Instance()->LodGenGenerate() )
	{
		if (m_pTaskPanel)
			m_pTaskPanel->TaskStarted();
	}
	return TRUE;
}

LRESULT CGeometryLodGeneratorDialog::OnCancel(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor wait;
	CLodGeneratorInteractionManager::Instance()->LogGenCancel();
	if (m_pTaskPanel)
		m_pTaskPanel->TaskFinished();
	return TRUE;
}

LRESULT CGeometryLodGeneratorDialog::OnGenerateLods(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor wait;
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
	int highestLod = pInstance->GetHighestLod();

	if ( highestLod > 0 )
	{
		if ( CryMessageBox("Lods already exist for this model, generating will overwrite any existing lods.","Overwrite existing lods?",MB_OKCANCEL) == IDCANCEL )
		{
			return TRUE;
		}
	}

	CLODGeneratorErrorGraphRamp* pRamp = m_pGeoGenPanel->GetRamp();
	const int lods = pRamp->GetHandleCount();

	pInstance->ClearUnusedLods(lods);

	int nSourceLod = pInstance->GetGeometryOption<int>("nSourceLod");
	for ( int idx = lods-1, count = 1; idx >= 0; --idx, ++count)
	{
		pInstance->GenerateLod(nSourceLod+count,pRamp->GetHandleData(idx).percentage);
	}
	pInstance->SaveSettings();
	pInstance->ReloadModel();

	OnMaterialGeneratePrepare();
	m_oRollupControl.ScrollToPage(3);

	return TRUE;
}

LRESULT CGeometryLodGeneratorDialog::OnLodRemoved(WPARAM wParam, LPARAM lParam)
{
	int nSourceLod = CLodGeneratorInteractionManager::Instance()->GetGeometryOption<int>("nSourceLod");
	int genLodCount = (int)wParam;
	int highestLod = CLodGeneratorInteractionManager::Instance()->GetHighestLod();

	if ( (highestLod - nSourceLod) > genLodCount )
		CryMessageBox("Deleting existing lods requires a full export of the lod chain and material baking pass.","LOD Tool Message",0);
	return TRUE;
}

LRESULT CGeometryLodGeneratorDialog::OnLodChainGenerationFinished(WPARAM wParam, LPARAM lParam)
{
	if(!m_pGeoGenPanel)
		return FALSE;

	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
	const int nSourceLod = pInstance->GetGeometryOption<int>("nSourceLod");
	const int nLods = pInstance->NumberOfLods() - nSourceLod;
	if ( nLods == 1 )
	{
		m_pGeoGenPanel->CreateLod(50.0f);
	}
	else
	{
		m_pGeoGenPanel->SelectFirst();
	}

	m_pGeoGenPanel->EnableExport();

	return TRUE;
}

void CGeometryLodGeneratorDialog::OnGeometryVarBlockChanged(IVariable* var)
{
	CString varName(var->GetName());
	if (varName.CompareNoCase("nSourceLod")==0)
	{
		Reset(false);
		m_pGeoGenPanel->CreateExistingLodKeys();
		OnMaterialGeneratePrepare();
	}
	else if (varName.CompareNoCase("bWireframe")==0)
	{
		bool value = false;
		var->Get(value);
		m_pGeoGenPanel->SetWireframe(value);
	}
	else if (varName.CompareNoCase("bSourceLod")==0)
	{
		bool value = false;
		var->Get(value);
		const int nSourceLod = CLodGeneratorInteractionManager::Instance()->GetGeometryOption<int>("nSourceLod");
		m_pGeoGenPanel->PreviewSource(value,nSourceLod);
	}
}

bool CGeometryLodGeneratorDialog::OnMaterialGeneratePrepare()
{
	if (!m_pFilePanel)
		return false;

	if (!m_pMatOptionsPanel)
		return false;

	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
	if(!pInstance->GetLoadedModel())
		return false;
	
	ClearLodPanels();
	GenerateLodPanels();
	m_pMatOptionsPanel->Update();

	XmlNodeRef settings = pInstance->GetSettings("MaterialLodSettings");
	const int individualLodChildSettings = settings->getChildCount();
	for (int idx = 0; idx < individualLodChildSettings; ++idx)
	{
		XmlNodeRef child = settings->getChild(idx);
		int nLodId = -1;
		if (!child->getAttr("nLodId",nLodId))
			break;

		const int nNumLods = m_vLodPanels.size();
		for (int panelidx = 0; panelidx < nNumLods; ++panelidx)
		{
			if (m_vLodPanels[panelidx]->LodId() == nLodId)
			{
				m_vLodPanels[panelidx]->Serialize(child,true);
				break;
			}
		}
	}

	return true;
}

LRESULT CGeometryLodGeneratorDialog::OnGenerateMaterial(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor wait;
	CLodGeneratorInteractionManager::Instance()->ClearResults();
	const int nNumLods = m_vLodPanels.size();
	for (int idx = 0; idx < nNumLods; ++idx)
	{
		CMaterialLODGeneratorLodItemOptionsPanel* pPanel = m_vLodPanels[idx];
		if (!pPanel->IsBakingEnabled())
			continue;

		int nLodId = pPanel->LodId();
		int nWidth = pPanel->Width();
		int nHeight = pPanel->Height();
		
		CLodGeneratorInteractionManager::Instance()->RunProcess(nLodId,nWidth,nHeight);
		pPanel->SetTextures();
	}
	OnSave();
	CLodGeneratorInteractionManager::Instance()->ClearResults();
	return TRUE;
}

void CGeometryLodGeneratorDialog::OnSave()
{
	CWaitCursor wait;
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();

	const int nNumLods = m_vLodPanels.size();
	for (int idx = 0; idx < nNumLods; ++idx)
	{
		CMaterialLODGeneratorLodItemOptionsPanel* pPanel = m_vLodPanels[idx];
		if (!pPanel->IsBakingEnabled())
			continue;

		int nLodId = pPanel->LodId();
		pInstance->SaveTextures(nLodId);
	}

	m_pMatOptionsPanel->Update();

	XmlNodeRef settings = pInstance->GetSettings("MaterialLodSettings");
	settings->removeAllChilds();
	for (int idx = 0; idx < nNumLods; ++idx)
	{
		CMaterialLODGeneratorLodItemOptionsPanel* pPanel = m_vLodPanels[idx];
		pPanel->Serialize(settings,false);
	}

	pInstance->SaveSettings();
}

LRESULT CGeometryLodGeneratorDialog::OnTextureSizeChanged(WPARAM nWidth, LPARAM nHeight)
{
	int curWidth = nWidth;
	int curHeight = nHeight;

	const int nNumLods = m_vLodPanels.size();
	for (int idx = 1; idx < nNumLods; ++idx)
	{
		curWidth = curWidth/2;
		curHeight = curHeight/2;
		m_vLodPanels[idx]->SetTextureSize(curWidth,curHeight);
	}
	return TRUE;
}

void CGeometryLodGeneratorDialog::ClearLodPanels()
{
	for (int idx = m_vLodPanelsIdx.size()-1; idx >= 0; --idx)
	{
		m_oRollupControl.RemovePage(m_vLodPanelsIdx[idx]);	
	}
	m_vLodPanelsIdx.clear();
	m_vLodPanels.clear();
}

void CGeometryLodGeneratorDialog::GenerateLodPanels()
{
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
	if (!pInstance)
		return;

	if (!m_pMatOptionsPanel)
		return;

	int	nSourceLod = pInstance->GetGeometryOption<int>("nSourceLod");

	const int nLods = pInstance->NumberOfLods();
	for (int idx = nSourceLod+1; idx < nLods; ++idx)
	{
		int nLodIdx = idx;
		int nSubmatIdx = pInstance->GetSubMatId(nLodIdx);
		bool bAllowTextureSizeControl = idx == nSourceLod+1;
		CMaterialLODGeneratorLodItemOptionsPanel* pItemPanel = new CMaterialLODGeneratorLodItemOptionsPanel(nLodIdx,nSubmatIdx,bAllowTextureSizeControl,&m_oRollupControl);
		if (!pItemPanel)
			continue;

		CString panelName;
		panelName.Format(CMaterialLODGeneratorLodItemOptionsPanel::kPanelCaption,idx);
		int idxPanel = m_oRollupControl.GetPagesCount();
		idxPanel = m_oRollupControl.InsertPage(panelName,pItemPanel,TRUE,idxPanel-1);
		m_vLodPanelsIdx.push_back(idxPanel);
		m_vLodPanels.push_back(pItemPanel);
	}

	for (int idx = 1; idx < nLods; idx++)
	{
		const int nSubMats = pInstance->GetSubMatCount(idx);
		if (nSubMats > 1)
		{
			CryMessageBox("Loaded lods have multiple sub-materials please ensure only a single material is used before attempting to bake texture materials", "LOD Sub-Material Warning", 0);
			break;
		}
	}
}

void CGeometryLodGeneratorDialog::Reset(bool bUpdateOptionPanels)
{
	ClearLodPanels();

	if (m_pTaskPanel)
		m_pTaskPanel->Reset();

	if(m_pGeoGenPanel)
		m_pGeoGenPanel->Reset(true);

	if(m_pMatTaskPanel)
		m_pMatTaskPanel->Reset();

	CLodGeneratorInteractionManager::Instance()->ResetSettings();
	if (!bUpdateOptionPanels)
		return;

	if (m_pOptionsPanel)
		m_pOptionsPanel->Update();

	if(m_pMatOptionsPanel)
		m_pMatOptionsPanel->Update();
}

//////////////////////////////////////////////////////////////////////////
// options panel
//////////////////////////////////////////////////////////////////////////

const char * CGeometryLodGeneratorOptionsPanel::kPanelCaption = "Geometry Bake Options";
CGeometryLodGeneratorOptionsPanel::CGeometryLodGeneratorOptionsPanel(CWnd* pParent)
{	
	m_pVarPanel = NULL;
	m_pToolTip = NULL;

	Create( IDD,pParent );
}

CGeometryLodGeneratorOptionsPanel::~CGeometryLodGeneratorOptionsPanel()
{
	SAFE_DELETE(m_pVarPanel);
	m_pToolTip = NULL;
}

BEGIN_MESSAGE_MAP(CGeometryLodGeneratorOptionsPanel, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CGeometryLodGeneratorOptionsPanel::OnInitDialog()
{
	BOOL ret = __super::OnInitDialog();

	CRect rc;
	GetClientRect(rc);

	m_pVarPanel = new CPropertiesPanel(this);
	m_pVarPanel->AddVars(CLodGeneratorInteractionManager::Instance()->GetGeometryVarBlock());
	m_pVarPanel->ModifyStyleEx(WS_EX_CLIENTEDGE|WS_EX_STATICEDGE|WS_EX_WINDOWEDGE,0,0);
	m_pVarPanel->ShowWindow(TRUE);

	m_pToolTip = new CToolTipCtrl();
	m_pToolTip->Create(this);
	m_pToolTip->AddTool(m_pVarPanel,"Setup the settings for creating the lod chain.");
	m_pToolTip->Activate(TRUE);

	return ret;
}

BOOL CGeometryLodGeneratorOptionsPanel::PreTranslateMessage(MSG* pMsg)
{
	if (m_pToolTip)
		m_pToolTip->RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}

void CGeometryLodGeneratorOptionsPanel::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	CRect rc;
	GetWindowRect(rc);
	ScreenToClient(rc);
	if ( m_pVarPanel )
		m_pVarPanel->MoveWindow(rc);
}

void CGeometryLodGeneratorOptionsPanel::Update()
{
	if (m_pVarPanel)
		m_pVarPanel->SetVarBlock(CLodGeneratorInteractionManager::Instance()->GetGeometryVarBlock());
}

//////////////////////////////////////////////////////////////////////////
// task panel
//////////////////////////////////////////////////////////////////////////
const char * CGeometryLodGeneratorTaskPanel::kPanelCaption = "Geometry Task Panel";

CGeometryLodGeneratorTaskPanel::CGeometryLodGeneratorTaskPanel(CWnd* pParent) 
{
	m_pToolTip = NULL;
	Create( IDD,pParent );
}

CGeometryLodGeneratorTaskPanel::~CGeometryLodGeneratorTaskPanel()
{
	m_pToolTip = NULL;
}

BEGIN_MESSAGE_MAP(CGeometryLodGeneratorTaskPanel, CDialog)
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_LOD_GEN_GENERATE, &CGeometryLodGeneratorTaskPanel::OnBnClickedLodGenGenerate)
	ON_BN_CLICKED(IDC_LOD_GEN_CANCEL, &CGeometryLodGeneratorTaskPanel::OnBnClickedLodGenCancel)
END_MESSAGE_MAP()

BOOL CGeometryLodGeneratorTaskPanel::OnInitDialog()
{
	BOOL ret = __super::OnInitDialog();

	m_pToolTip = new CToolTipCtrl();
	m_pToolTip->Create(this);

	Reset();

	m_pToolTip->AddTool(GetDlgItem(IDC_LOD_GEN_GENERATE),"Begin generating the LOD chain.");
	m_pToolTip->AddTool(GetDlgItem(IDC_LOD_GEN_CANCEL),"Cancels the current LOD chain being generated.");
	m_pToolTip->AddTool(GetDlgItem(IDC_LOD_GEN_PROGRESS),"Displays the estimated progress of the current LOD chain generation.");

	m_pToolTip->Activate(TRUE);

	return ret;
}

BOOL CGeometryLodGeneratorTaskPanel::PreTranslateMessage(MSG* pMsg)
{
	if (m_pToolTip)
		m_pToolTip->RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}

void CGeometryLodGeneratorTaskPanel::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType,cx,cy);

	int ids[]={IDC_LOD_GEN_GENERATE, IDC_LOD_GEN_PROGRESS, IDC_LOD_GEN_CANCEL, IDC_STATIC_LOD_GEN_PROGRESS_TEXT};
	for (int i=0; i<sizeof(ids)/sizeof(ids[0]); i++)
	{
		CWnd *pWnd=GetDlgItem(ids[i]);
		if (pWnd)
		{
			CRect rc;
			GetClientRect(rc);
			int left=rc.left;
			int right=rc.right;
			pWnd->GetWindowRect(rc);
			ScreenToClient(rc);
			rc.left=left;
			rc.right=right;
			pWnd->MoveWindow(rc, FALSE);
		}
	}
}

void CGeometryLodGeneratorTaskPanel::OnBnClickedLodGenGenerate()
{
	GetParent()->GetParent()->PostMessage(WM_GEOM_LOD_CHAIN_GENERATE);
}

void CGeometryLodGeneratorTaskPanel::OnBnClickedLodGenCancel()
{
	GetParent()->GetParent()->PostMessage(WM_GEOM_LOD_CHAIN_CANCEL);
	Reset();
}

void CGeometryLodGeneratorTaskPanel::OnTimer(UINT_PTR nIDEvent)
{
	float fProgress = 0.0f;
	float fProgressPercentage = 0.0f;
	CString text;
	bool bFinished = false;

	if ( CLodGeneratorInteractionManager::Instance()->LodGenGenerateTick(&fProgress) )
	{
		text.Format("Finished!");
		bFinished = true;
		TaskFinished();
		GetParent()->GetParent()->PostMessage(WM_GEOM_LOD_CHAIN_GENERATION_FINISHED);
	}
	else
	{
		bFinished = false;
		fProgressPercentage = 100.0f*fProgress;
		if ( fProgressPercentage == 0.0f )
		{
			text.Format("Estimating...");
		}
		else
		{
			CTime time = CTime::GetCurrentTime();
			CTimeSpan timeSpan = (time - m_tStartTime);

			float fExpendedTime = (float)timeSpan.GetTotalSeconds();
			float fTotalSeconds = (fExpendedTime / fProgressPercentage) * 100.0f;
			float fTimeLeft = fTotalSeconds - fExpendedTime;
			int Minutes = (int)(fTimeLeft / 60.0f);
			int Seconds = (((int)fTimeLeft) % 60);
			text.Format("ETA %d Minutes %d Seconds Remaining", Minutes, Seconds);
		}
	}

	SetDlgItemText(IDC_STATIC_LOD_GEN_PROGRESS_TEXT, text);

	CProgressCtrl* pProgress=(CProgressCtrl*)GetDlgItem(IDC_LOD_GEN_PROGRESS);
	if (pProgress)
	{
		pProgress->EnableWindow(bFinished);
		pProgress->SetPos((int)fProgressPercentage);
	}

	__super::OnTimer(nIDEvent);
}

void CGeometryLodGeneratorTaskPanel::Reset()
{
	CProgressCtrl *pProgress=(CProgressCtrl*)GetDlgItem(IDC_LOD_GEN_PROGRESS);
	pProgress->SetPos(0);
	pProgress->EnableWindow(FALSE);
	pProgress->SetRange(0,100);
	
	CStatic *pProgressText=(CStatic*)GetDlgItem(IDC_STATIC_LOD_GEN_PROGRESS_TEXT);
	CString text;
	text.Format("Waiting for task..");
	pProgressText->SetWindowText(text);

	TaskFinished();
}

void CGeometryLodGeneratorTaskPanel::TaskStarted()
{
	m_tStartTime = CTime::GetCurrentTime();
	GetDlgItem(IDC_LOD_GEN_CANCEL)->EnableWindow(TRUE);
	GetDlgItem(IDC_LOD_GEN_CANCEL)->ShowWindow(TRUE);
	GetDlgItem(IDC_LOD_GEN_GENERATE)->EnableWindow(FALSE);
	GetDlgItem(IDC_LOD_GEN_GENERATE)->ShowWindow(FALSE);
	m_nTimer = SetTimer(1,1000,0);
}

void CGeometryLodGeneratorTaskPanel::TaskFinished()
{
	GetDlgItem(IDC_LOD_GEN_CANCEL)->EnableWindow(FALSE);
	GetDlgItem(IDC_LOD_GEN_CANCEL)->ShowWindow(FALSE);
	GetDlgItem(IDC_LOD_GEN_GENERATE)->EnableWindow(TRUE);
	GetDlgItem(IDC_LOD_GEN_GENERATE)->ShowWindow(TRUE);
	KillTimer(m_nTimer);
}

//////////////////////////////////////////////////////////////////////////
// Geometry preview Panel
//////////////////////////////////////////////////////////////////////////

const char * CGeometryLodGeneratorPreviewPanel::kPanelCaption = "Geometry Generation Panel";

CGeometryLodGeneratorPreviewPanel::CGeometryLodGeneratorPreviewPanel(CWnd* pParent)
{
	m_pPreview = NULL;
	m_pPreviewPopup = NULL;
	m_pRamp = new CLODGeneratorErrorGraphRamp();
	m_pToolTip = NULL;

	Create( IDD,pParent );
}

CGeometryLodGeneratorPreviewPanel::~CGeometryLodGeneratorPreviewPanel()
{
	SAFE_DELETE(m_pPreview);
	SAFE_DELETE(m_pRamp);
	SAFE_DELETE(m_pPreviewPopup);
	m_pToolTip = NULL;
}

BEGIN_MESSAGE_MAP(CGeometryLodGeneratorPreviewPanel, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_LOD_GEN_UVMAP_AND_SAVE, &CGeometryLodGeneratorPreviewPanel::OnGenerateLods)
	ON_REGISTERED_MESSAGE(WM_HANDLECHANGED, OnLodValueChanged)
	ON_REGISTERED_MESSAGE(WM_HANDLEDELETED, OnLodDeleted)
	ON_REGISTERED_MESSAGE(WM_HANDLEADDED, OnLodAdded)
	ON_REGISTERED_MESSAGE(WM_HANDLESELECTED, OnLodSelected)
	ON_REGISTERED_MESSAGE(WM_HANDLESELECTIONCLEARED, OnSelectionCleared)
END_MESSAGE_MAP()

BOOL CGeometryLodGeneratorPreviewPanel::OnInitDialog()
{
	BOOL ret = __super::OnInitDialog();

	m_pToolTip = new CToolTipCtrl();
	m_pToolTip->Create(this);

	CWnd *pControl=GetDlgItem(IDC_LOD_GEN_PREVIEW);
	CRect lrc;
	pControl->GetWindowRect(lrc);
	ScreenToClient(lrc);
	
	m_pPreview=new CMeshBakerPopupPreview();
	m_pPreview->Create(this, lrc, WS_CHILD|WS_VISIBLE);
	m_pPreview->SetRotate(false);
	m_pPreview->SetWireframe(true);
	
	m_pRamp->SetParent(this);
	m_pRamp->SetOwner(this);
	
	Reset(true);

	m_pToolTip->AddTool(m_pPreview,"This viewport allows you to preview to new lod geometry before generating the lod files.");
	m_pToolTip->AddTool(m_pRamp,"Each key on the ramp will generate a lod at its value. Value range is 1% - 100%");
	m_pToolTip->AddTool(GetDlgItem(IDC_LOD_GEN_UVMAP_AND_SAVE), "Generates the LOD models with UVs and saves the files to disk.");

	m_pToolTip->Activate(TRUE);
	return ret;
}

BOOL CGeometryLodGeneratorPreviewPanel::PreTranslateMessage(MSG* pMsg)
{
	if (m_pToolTip)
		m_pToolTip->RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}

void CGeometryLodGeneratorPreviewPanel::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX,IDC_LODCHAIN_RAMP,*m_pRamp);
	CDialog::DoDataExchange(pDX);
}

void CGeometryLodGeneratorPreviewPanel::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	Reset(false);
}

void CGeometryLodGeneratorPreviewPanel::OnGenerateLods()
{
	GetParent()->GetParent()->PostMessage(WM_GEOM_LOD_GENERATE_LODS);

	CButton * pButton = (CButton*)GetDlgItem(IDC_LOD_GEN_MATERIAL_TEXTURES);
	if (pButton)
		pButton->EnableWindow(TRUE);
}

void CGeometryLodGeneratorPreviewPanel::Reset(bool release)
{
	if ( release )
	{
		if (m_pPreview)
			m_pPreview->Reset();

		CWnd *pControlWnd = GetDlgItem(IDC_LOD_GEN_MATERIAL_TEXTURES);
		if (pControlWnd)
			pControlWnd->EnableWindow(FALSE);

		CButton * pButton = (CButton*)GetDlgItem(IDC_LOD_GEN_UVMAP_AND_SAVE);
		if (pButton)
			pButton->EnableWindow(FALSE);

		if (m_pRamp)
			m_pRamp->Reset();
	}

	int border=4;
	CRect lrc;
	CRect rc;
	GetClientRect(rc);

	CMeshBakerPopupPreview *pControl=m_pPreview;
	if (pControl)
	{
		pControl->GetWindowRect(lrc);
		ScreenToClient(lrc);
		lrc.left=rc.left+border;
		lrc.right=rc.right-border;
		pControl->MoveWindow(lrc, FALSE);
	}

	if ( m_pRamp && m_pRamp->GetSafeHwnd() )
	{
		m_pRamp->GetWindowRect(lrc);
		ScreenToClient(lrc);
		lrc.left = rc.left+border;
		lrc.right = rc.right-border;
		m_pRamp->MoveWindow(lrc, FALSE);
	}

	CWnd* pButton = GetDlgItem(IDC_LOD_GEN_UVMAP_AND_SAVE);
	if (pButton)
	{
		lrc.top = lrc.bottom + border;
		lrc.bottom = lrc.top + 24;
		lrc.left = rc.left+border;
		lrc.right = rc.right-border;
		pButton->MoveWindow(lrc, FALSE);
	}

	Invalidate();
}

void CGeometryLodGeneratorPreviewPanel::SetNumLods(int nSourceLod)
{
	int nMaxLods = CLodGeneratorInteractionManager::Instance()->GetMaxNumLods();
	if (m_pRamp)
		m_pRamp->SetMaxHandles(nMaxLods-nSourceLod);
}

void CGeometryLodGeneratorPreviewPanel::PreviewSource(bool bDisplay, int nSourceLod)
{
	if ( !bDisplay )
	{
		SAFE_DELETE(m_pPreviewPopup)
		return;
	}
	else 
	{
		CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
		CRect rc;
		GetWindowRect(&rc);
		int size=(rc.right-rc.left) / 4;
		rc.right=rc.left+size;
		rc.bottom=rc.top+size;
		m_pPreviewPopup=new CMeshBakerPopupPreview();
		m_pPreviewPopup->Create(this, rc, WS_VISIBLE|WS_POPUPWINDOW);
		const int nSourceLod = pInstance->GetGeometryOption<int>("nSourceLod");
		m_pPreviewPopup->SetModel(pInstance->GetLoadedModel(nSourceLod));
		m_pPreviewPopup->SetRotate(false);
		::SetWindowPos(m_pPreviewPopup->GetSafeHwnd(),HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
		m_pPreviewPopup->SetFocus();
	}
}

void CGeometryLodGeneratorPreviewPanel::SetWireframe(bool bWireframe)
{
	m_pPreview->SetWireframe(bWireframe);

	if (!bWireframe)
	{
		m_pPreview->SetMaterial(NULL);
	}
	else
	{
		CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
		if (CMaterial* pMat = pInstance->LoadSpecificMaterial("materials/basecolors/base_black") )
			m_pPreview->SetMaterial(pMat);
	}
}

void CGeometryLodGeneratorPreviewPanel::CreateExistingLodKeys()
{
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();

	if (!pInstance)
		return;

	if (!m_pRamp)
		return;

	m_pRamp->SelectAllHandles();
	m_pRamp->RemoveSelectedHandles();

	const int nSourceLod = pInstance->GetGeometryOption<int>("nSourceLod");
	const int nLods = pInstance->NumberOfLods();
	for ( int nLodIdx = nSourceLod+1; nLodIdx < nLods; ++nLodIdx )
	{
		m_pRamp->AddHandle(pInstance->GetLodsPercentage(nLodIdx));
	}
}

LRESULT CGeometryLodGeneratorPreviewPanel::OnLodValueChanged(WPARAM wParam, LPARAM lParam)
{
	CRampControl::HandleData data;
	if ( m_pRamp && m_pRamp->GetSelectedData(data) )
	{
		CLodGeneratorInteractionManager::Instance()->GenerateTemporaryLod(data.percentage,m_pPreview->GetModelCtrl(),m_pRamp);
	}
	return TRUE;
}

LRESULT CGeometryLodGeneratorPreviewPanel::OnLodDeleted(WPARAM wParam, LPARAM lParam)
{
	if (m_pRamp)
	{
		int nLodCount = m_pRamp->GetHandleCount();
		GetParent()->GetParent()->PostMessage(WM_GEOM_LOD_REMOVED, WPARAM(nLodCount));
		Invalidate();
	}
	return TRUE;
}

LRESULT CGeometryLodGeneratorPreviewPanel::OnLodAdded(WPARAM wParam, LPARAM lParam)
{
	CLodGeneratorInteractionManager::Instance()->GenerateTemporaryLod((float)wParam,m_pPreview->GetModelCtrl(),m_pRamp);
	Invalidate();
	return TRUE;
}

LRESULT CGeometryLodGeneratorPreviewPanel::OnLodSelected(WPARAM wParam, LPARAM lParam)
{
	CRampControl::HandleData data;
	if (m_pRamp && m_pRamp->GetSelectedData(data) )
	{
		CLodGeneratorInteractionManager::Instance()->GenerateTemporaryLod(data.percentage,m_pPreview->GetModelCtrl(),m_pRamp);
		Invalidate();
	}
	return TRUE;
}

LRESULT CGeometryLodGeneratorPreviewPanel::OnSelectionCleared(WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

void CGeometryLodGeneratorPreviewPanel::CreateLod(float fPercentage)
{
	if (!m_pRamp)
		return;

	m_pRamp->AddHandle(fPercentage);
	m_pRamp->SelectHandle(fPercentage);
	PostMessage(WM_HANDLESELECTED);
}

void CGeometryLodGeneratorPreviewPanel::SelectFirst()
{
	if (!m_pRamp)
		return;

	m_pRamp->SelectNextHandle();
	PostMessage(WM_HANDLESELECTED);
}

void CGeometryLodGeneratorPreviewPanel::EnableExport()
{
	CWnd *pControlWnd = GetDlgItem(IDC_LOD_GEN_UVMAP_AND_SAVE);
	if (pControlWnd)
		pControlWnd->EnableWindow(TRUE);
}