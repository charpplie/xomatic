/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2013.
*************************************************************************/

#include "stdafx.h"
#include "MaterialLODGeneratorDialog.h"
#include "LODInterface.h"
#include "LODGeneratorSharedUI.h"
#include "PropertiesPanel.h"

//////////////////////////////////////////////////////////////////////////
// options panel
//////////////////////////////////////////////////////////////////////////

const char * CMaterialLODGeneratorOptionsPanel::kPanelCaption = "Material Bake Options";
CMaterialLODGeneratorOptionsPanel::CMaterialLODGeneratorOptionsPanel(CWnd* pParent)
{	
	m_pVarPanel = NULL;
	m_pToolTip = NULL;
	Create( IDD,pParent );
}

CMaterialLODGeneratorOptionsPanel::~CMaterialLODGeneratorOptionsPanel()
{
	SAFE_DELETE(m_pVarPanel);
	m_pToolTip = NULL;
}

BEGIN_MESSAGE_MAP(CMaterialLODGeneratorOptionsPanel, CDialog)
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CMaterialLODGeneratorOptionsPanel::OnInitDialog()
{
	BOOL ret = __super::OnInitDialog();
	
	CRect rc;
	GetClientRect(rc);

	m_pVarPanel = new CPropertiesPanel(this);
	m_pVarPanel->AddVars(CLodGeneratorInteractionManager::Instance()->GetMaterialVarBlock());
	m_pVarPanel->ModifyStyleEx(WS_EX_CLIENTEDGE|WS_EX_STATICEDGE|WS_EX_WINDOWEDGE,0,0);
	m_pVarPanel->ShowWindow(TRUE);
	
	m_pToolTip = new CToolTipCtrl();
	m_pToolTip->Create(this);
	m_pToolTip->AddTool(m_pVarPanel,"Setup the settings for creating the lod materials here.");
	m_pToolTip->Activate(TRUE);

	return ret;
}

void CMaterialLODGeneratorOptionsPanel::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	CRect rc;
	GetWindowRect(rc);
	ScreenToClient(rc);
	if ( m_pVarPanel )
		m_pVarPanel->MoveWindow(rc);
}

BOOL CMaterialLODGeneratorOptionsPanel::PreTranslateMessage(MSG* pMsg)
{
	if (m_pToolTip)
		m_pToolTip->RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}

void CMaterialLODGeneratorOptionsPanel::Update()
{
	if (m_pVarPanel)
		m_pVarPanel->SetVarBlock(CLodGeneratorInteractionManager::Instance()->GetMaterialVarBlock());
}

//////////////////////////////////////////////////////////////////////////
// lods options panel
//////////////////////////////////////////////////////////////////////////

const char * CMaterialLODGeneratorLodItemOptionsPanel::kPanelCaption = "LOD (%d) Options Panel";
CMaterialLODGeneratorLodItemOptionsPanel::CMaterialLODGeneratorLodItemOptionsPanel(int nLodId, int nSubmatId, bool bAllowControl, CWnd* pParent)
{	
	m_nLodId = nLodId;
	m_nSubMaterialId = nSubmatId;
	m_bAllowControl = bAllowControl;
	m_pToolTip = NULL;
	Create( IDD,pParent );
}

CMaterialLODGeneratorLodItemOptionsPanel::~CMaterialLODGeneratorLodItemOptionsPanel()
{
	SAFE_DELETE(m_pCagePreview);
	m_pToolTip = NULL;
}

BEGIN_MESSAGE_MAP(CMaterialLODGeneratorLodItemOptionsPanel, CDialog)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_COMMAND(IDC_CHECK_MAT_LOD_BAKE_LOD,OnEnabledChanged)
END_MESSAGE_MAP()


void CMaterialLODGeneratorLodItemOptionsPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_MAT_LOD_BAKE_LOD, m_bEnabled);
	DDX_Control(pDX, IDC_PREVIEW_MAT_LOD_CAGE, m_cagePreviewDummy);
}

BOOL CMaterialLODGeneratorLodItemOptionsPanel::OnInitDialog()
{
	BOOL ret = __super::OnInitDialog();
		
	CSliderCtrl* pSlider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_MAT_LOD_OUTPUT_SIZE);
	pSlider->SetRange(10, 22);
	pSlider->SetPos(18-((m_nLodId-1)*2)); //start at 512x512 and drop down each lod id
	pSlider->EnableWindow(m_bAllowControl);
	pSlider->ShowWindow(m_bAllowControl);

	UpdateSizeControl(false);
		
	CString slot;
	slot.Format("%d",m_nSubMaterialId+1);
	CWnd* pSubMatEdit = (CWnd*)GetDlgItem(IDC_EDIT_MAT_LOD_SUBMAT_SLOT);
	pSubMatEdit->SetWindowTextA(slot);

	CWnd *pControl=GetDlgItem(IDC_PREVIEW_MAT_LOD_CAGE);
	CRect lrc;
	pControl->GetWindowRect(lrc);
	ScreenToClient(lrc);
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
	m_pCagePreview = new CMeshBakerPopupPreview();
	m_pCagePreview->Create(this,lrc,WS_VISIBLE|WS_CHILD);
	m_pCagePreview->SetModel(pInstance->GetLoadedModel());
	m_pCagePreview->SetMaterial(pInstance->GetLoadedMaterial());
	m_pCagePreview->SetRotate(false);
	m_pCagePreview->SetGrid(false);
	
	pControl=GetDlgItem(IDC_PREVIEW_DIFFUSE);
	pControl->GetWindowRect(lrc);
	ScreenToClient(lrc);
	m_colour.Create(this,lrc,WS_CHILD|WS_VISIBLE);

	pControl=GetDlgItem(IDC_PREVIEW_NORMAL);
	pControl->GetWindowRect(lrc);
	ScreenToClient(lrc);
	m_normal.Create(this,lrc,WS_CHILD|WS_VISIBLE);

	pControl=GetDlgItem(IDC_PREVIEW_SPEC);
	pControl->GetWindowRect(lrc);
	ScreenToClient(lrc);
	m_spec.Create(this,lrc,WS_CHILD|WS_VISIBLE);

	m_bEnabled = true;
	UpdateData(FALSE);

	m_pToolTip = new CToolTipCtrl();
	m_pToolTip->Create(this);
	m_pToolTip->AddTool(pSlider,"Controls the texture sizes for new textures that will be generated.");
	m_pToolTip->AddTool(pSubMatEdit,"Sub Material slot the generated textures will be assigned too.");
	m_pToolTip->AddTool(m_pCagePreview, "Preview of the LOD cage used to bake the textures down for this Sub Material.");
	m_pToolTip->AddTool(GetDlgItem(IDC_PREVIEW_DIFFUSE), "Preview of the diffuse texture that has been generated. Hover to expand.");
	m_pToolTip->AddTool(GetDlgItem(IDC_PREVIEW_NORMAL), "Preview of the normal map texture that has been generated. Hover to expand.");
	m_pToolTip->AddTool(GetDlgItem(IDC_PREVIEW_SPEC), "Preview of the specular map texture that has been generated. Hover to expand.");
	m_pToolTip->Activate(TRUE);

	return ret;
}

void CMaterialLODGeneratorLodItemOptionsPanel::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
}

BOOL CMaterialLODGeneratorLodItemOptionsPanel::PreTranslateMessage(MSG* pMsg)
{
	if (m_pToolTip)
		m_pToolTip->RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}

void CMaterialLODGeneratorLodItemOptionsPanel::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollbar)
{
	UpdateSizeControl(true);
}

void CMaterialLODGeneratorLodItemOptionsPanel::OnEnabledChanged()
{
	UpdateData(TRUE);
}

bool CMaterialLODGeneratorLodItemOptionsPanel::IsBakingEnabled()
{
	return (bool)m_bEnabled;
}

int CMaterialLODGeneratorLodItemOptionsPanel::Width()
{
	CSliderCtrl* pSizeSlider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_MAT_LOD_OUTPUT_SIZE);
	int outputHeight = 1<<(pSizeSlider->GetPos()/2);
	int outputWidth = outputHeight;
	if (pSizeSlider->GetPos()&1)
		outputWidth <<= 1;
	return outputWidth;
}

int CMaterialLODGeneratorLodItemOptionsPanel::Height()
{
	CSliderCtrl* pSizeSlider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_MAT_LOD_OUTPUT_SIZE);
	int outputHeight = 1<<(pSizeSlider->GetPos()/2);
	return outputHeight;
}

int CMaterialLODGeneratorLodItemOptionsPanel::LodId()
{
	return m_nLodId;
}

int CMaterialLODGeneratorLodItemOptionsPanel::SubMatId()
{
	return m_nSubMaterialId;
}

void CMaterialLODGeneratorLodItemOptionsPanel::UpdateSizeControl(bool bUsrMsg)
{
	CSliderCtrl* pControl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_MAT_LOD_OUTPUT_SIZE);
	CEdit* pLabel = (CEdit*)GetDlgItem(IDC_EDIT_MAT_LOD_TEXTURE_SIZE);
	
	CString labelText;
	labelText.Format("%dx%d", Width(), Height());
	pLabel->SetWindowTextA(labelText);

	if (bUsrMsg)
		GetParent()->GetParent()->PostMessage(WM_MAT_LOD_TEXTURESIZE_CHANGED,(WPARAM)Width(),(LPARAM)Height());
}

void CMaterialLODGeneratorLodItemOptionsPanel::SetTextures()
{
	const SMeshBakingOutput* pResults = CLodGeneratorInteractionManager::Instance()->GetResults(m_nLodId);
	if (!pResults)
		return;

	for(int texid = CLodGeneratorInteractionManager::eTextureType_Diffuse; texid < CLodGeneratorInteractionManager::eTextureType_Max; ++texid)
	{
		SetTexture(pResults->ppOuputTexture[texid],(CLodGeneratorInteractionManager::eTextureType)texid);
	}
}

void CMaterialLODGeneratorLodItemOptionsPanel::SetTexture(ITexture* pTex, int type)
{
	switch (type)
	{
		case CLodGeneratorInteractionManager::eTextureType_Diffuse: m_colour.SetTexture(pTex, false);
			break;
		case CLodGeneratorInteractionManager::eTextureType_Normal: m_normal.SetTexture(pTex, false);
			break;
		case CLodGeneratorInteractionManager::eTextureType_Spec: m_spec.SetTexture(pTex, false);
			break;
	}
}

void CMaterialLODGeneratorLodItemOptionsPanel::SetTextureSize(int nWidth, int nHeight)
{
	int pos = (logf((float)nWidth) / logf((float)2))*2;
	CSliderCtrl* pControl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_MAT_LOD_OUTPUT_SIZE);
	pControl->SetPos(pos);
	UpdateSizeControl(false);
}

void CMaterialLODGeneratorLodItemOptionsPanel::Serialize(XmlNodeRef xml,bool load)
{
	if ( !load )
	{
		XmlNodeRef ref = xml->createNode("IndividualLodSettings");
		ref->setAttr("nLodId",m_nLodId);
		ref->setAttr("nWidth",Width());
		ref->setAttr("nHeight",Height());
		ref->setAttr("bEnabled",IsBakingEnabled());
		xml->addChild(ref);
	}
	else
	{
		int nWidth = 0;
		int nHeight = 0;
		xml->getAttr("nLodId", m_nLodId);
		xml->getAttr("bEnabled", m_bEnabled);
		xml->getAttr("nWidth",nWidth);
		xml->getAttr("nHeight",nHeight);
		SetTextureSize(nWidth,nHeight);
		UpdateData(FALSE);
	}
}

//////////////////////////////////////////////////////////////////////////
// Material lod generator task
//////////////////////////////////////////////////////////////////////////
const char * CMaterialLODGeneratorTaskPanel::kPanelCaption = "Material Generation Panel";
CMaterialLODGeneratorTaskPanel::CMaterialLODGeneratorTaskPanel(CWnd * pParent)
{
	m_pToolTip = NULL;
	Create(IDD,pParent);
}

CMaterialLODGeneratorTaskPanel::~CMaterialLODGeneratorTaskPanel()
{
	m_pToolTip = NULL;
}

BEGIN_MESSAGE_MAP(CMaterialLODGeneratorTaskPanel, CDialog)
	ON_WM_SIZE()
	ON_COMMAND(IDC_MAT_LOD_GENERATE, OnButtonGenerate)
END_MESSAGE_MAP()

BOOL CMaterialLODGeneratorTaskPanel::OnInitDialog()
{
	BOOL ret = __super::OnInitDialog();
	
	m_pToolTip = new CToolTipCtrl();
	m_pToolTip->Create(this);
	m_pToolTip->AddTool(GetDlgItem(IDC_MAT_LOD_GENERATE),"Generates textures for each sub material and lod specified in the panels above.");
	m_pToolTip->Activate(TRUE);

	return ret;
}

void CMaterialLODGeneratorTaskPanel::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType,cx,cy);
	
	int border=4;
	CRect lrc;
	CRect rc;
	GetClientRect(rc);

	CWnd* pControl = GetDlgItem(IDC_MAT_LOD_GENERATE);
	if (pControl)
	{
		pControl->GetWindowRect(lrc);
		ScreenToClient(lrc);
		lrc.left=rc.left+border;
		lrc.right=rc.right-border;
		pControl->MoveWindow(lrc, FALSE);
	}
}

BOOL CMaterialLODGeneratorTaskPanel::PreTranslateMessage(MSG* pMsg)
{
	if (m_pToolTip)
		m_pToolTip->RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}


void CMaterialLODGeneratorTaskPanel::OnButtonGenerate()
{
	GetParent()->GetParent()->PostMessage(WM_MAT_LOD_GENERATE);
}

void CMaterialLODGeneratorTaskPanel::Reset()
{

}
