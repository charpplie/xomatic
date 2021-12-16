////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AssetBrowserPreviewCharacterDlg.h"
#include "AssetCharacterItem.h"
#include "Asset Browser/AssetBrowserDialog.h"
#include "AssetBrowserPreviewCharacterDlgFooter.h"

IMPLEMENT_DYNAMIC(CAssetBrowserPreviewCharacterDlg, CDialog)

CAssetBrowserPreviewCharacterDlg::CAssetBrowserPreviewCharacterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserPreviewCharacterDlg::IDD, pParent)
{
	m_pModel = NULL;
	m_pFooter = NULL;
}

CAssetBrowserPreviewCharacterDlg::~CAssetBrowserPreviewCharacterDlg()
{
	m_pModel = NULL;
	m_pFooter = NULL;
}

void CAssetBrowserPreviewCharacterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ASSET_BROWSE_MODEL_LIGHTING_COMBO, m_lightingCB);
	DDX_Control(pDX, IDC_BUTTON_FULLSCREEN, m_fullscreenBtn);
}

BEGIN_MESSAGE_MAP(CAssetBrowserPreviewCharacterDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_WIREFRAME, OnBnClickedButtonWireframe)
	ON_BN_CLICKED(IDC_CHECK_PHYSICS, OnBnClickedButtonPhysics)
	ON_BN_CLICKED(IDC_CHECK_NORMALS, OnBnClickedButtonNormals)
	ON_BN_CLICKED(IDC_BUTTON_RESET_VIEW, OnBnClickedButtonResetView)
	ON_BN_CLICKED(IDC_BUTTON_FULLSCREEN, OnBnClickedButtonFullscreen)
	ON_CBN_SELCHANGE(IDC_ASSET_BROWSE_MODEL_LIGHTING_COMBO, OnLightingChanged)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_THUMB_ANGLE, OnBnClickedButtonSaveThumbAngle)
END_MESSAGE_MAP()

BOOL CAssetBrowserPreviewCharacterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	XmlNodeRef lightingNodeDay = GetISystem()->LoadXmlFromFile("Editor/asset_model_preview_day.tod");
	XmlNodeRef lightingNodeEvening = GetISystem()->LoadXmlFromFile("Editor/asset_model_preview_evening.tod");
	XmlNodeRef lightingNodeNight = GetISystem()->LoadXmlFromFile("Editor/asset_model_preview_night.tod");

	if (lightingNodeDay)
	{
		m_lightingMap.insert(std::pair<CString, XmlNodeRef>("Day", lightingNodeDay));
	}

	if (lightingNodeEvening)
	{
		m_lightingMap.insert(std::pair<CString, XmlNodeRef>("Evening", lightingNodeEvening));
	}

	if (lightingNodeNight)
	{
		m_lightingMap.insert(std::pair<CString, XmlNodeRef>("Night", lightingNodeNight));
	}

	m_oldTod = GetISystem()->CreateXmlNode();
	ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();

	if (pTimeOfDay)
	{
		pTimeOfDay->Serialize(m_oldTod, false);
	}

	for (std::map<CString, XmlNodeRef>::iterator item = m_lightingMap.begin(), end = m_lightingMap.end(); item != end; ++item)
	{
		m_lightingCB.AddString(item->first);
	}

	m_lightingCB.SetCurSel(0);
	m_fullscreenBtn.SetBitmap(::LoadBitmap(AfxGetResourceHandle(), MAKEINTRESOURCE(IDB_MODEL_PREVIEW_FULLSCREEN)));
	UpdateData(FALSE);
	OnLightingChanged();

	return TRUE;
}

// CAssetBrowserPreviewCharacterDlg message handlers
void CAssetBrowserPreviewCharacterDlg::Init()
{
	CheckDlgButton(IDC_CHECK_WIREFRAME, CAssetCharacterItem::s_bWireframe);
	CheckDlgButton(IDC_CHECK_PHYSICS, CAssetCharacterItem::s_bPhysics);
	CheckDlgButton(IDC_CHECK_NORMALS, CAssetCharacterItem::s_bNormals);
}

void CAssetBrowserPreviewCharacterDlg::OnBnClickedButtonWireframe()
{
	CAssetCharacterItem::s_bWireframe = IsDlgButtonChecked(IDC_CHECK_WIREFRAME) == BST_CHECKED;

	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewCharacterDlg::OnBnClickedButtonPhysics()
{
	CAssetCharacterItem::s_bPhysics = IsDlgButtonChecked(IDC_CHECK_PHYSICS) == BST_CHECKED;

	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewCharacterDlg::OnBnClickedButtonNormals()
{
	CAssetCharacterItem::s_bNormals = IsDlgButtonChecked(IDC_CHECK_NORMALS) == BST_CHECKED;

	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewCharacterDlg::OnBnClickedButtonResetView()
{
	if (m_pModel)
	{
		CheckDlgButton(IDC_CHECK_WIREFRAME, (CAssetCharacterItem::s_bWireframe = false));
		CheckDlgButton(IDC_CHECK_PHYSICS, (CAssetCharacterItem::s_bPhysics = false));
		CheckDlgButton(IDC_CHECK_NORMALS, (CAssetCharacterItem::s_bNormals = false));
		DockViewPane();
		m_pModel->ResetView();

		if (m_pFooter)
		{
			m_pFooter->Reset();
		}

		m_lightingCB.SetCurSel(0);
		OnLightingChanged();
	}

	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewCharacterDlg::OnLightingChanged()
{
	CString combotext;

	GetDlgItemTextA(IDC_ASSET_BROWSE_MODEL_LIGHTING_COMBO, combotext);

	CString lightingItem(combotext);
	std::map<CString, XmlNodeRef>::iterator item = m_lightingMap.find(lightingItem);

	if (item != m_lightingMap.end())
	{
		ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();

		if (pTimeOfDay && item->second)
		{
			pTimeOfDay->Serialize(item->second, true);
		}
	}
}

CXTPDockingPaneManager* CAssetBrowserPreviewCharacterDlg::GetDockingManager()
{
	CAssetBrowserDialog* pBrowser = (CAssetBrowserDialog*)GetIEditor()->FindView(ASSET_BROWSER_NAME);

	if (!pBrowser)
	{
		return NULL;
	}

	CXTPDockingPaneManager* pDockingManager = pBrowser->GetDockingPaneManager();

	if (!pDockingManager)
	{
		return NULL;
	}

	return pDockingManager;
}

CXTPDockingPane* CAssetBrowserPreviewCharacterDlg::GetDockPane(CXTPDockingPaneManager* pDockingManager)
{
	if (!pDockingManager)
	{
		return NULL;
	}

	CXTPDockingPane* previewPane = pDockingManager->FindPane(IDW_ASSET_BROWSER_PREVIEW_PANE);

	if (!previewPane)
	{
		return NULL;
	}

	return previewPane;
}

void CAssetBrowserPreviewCharacterDlg::OnBnClickedButtonFullscreen()
{
	CXTPDockingPaneManager* pDockingManager = GetDockingManager();

	if (!pDockingManager)
	{
		return;
	}

	CXTPDockingPane* previewPane = GetDockPane(pDockingManager);

	if (previewPane)
	{
		pDockingManager->ToggleDocking(previewPane);
	}
}

void CAssetBrowserPreviewCharacterDlg::DockViewPane()
{
	CXTPDockingPaneManager* pDockingManager = GetDockingManager();

	if (!pDockingManager)
	{
		return;
	}

	CXTPDockingPane* previewPane = GetDockPane(pDockingManager);

	if (previewPane)
	{
		if (previewPane->IsFloating())
		{
			pDockingManager->ToggleDocking(previewPane);
		}
	}
}

void CAssetBrowserPreviewCharacterDlg::OnBnClickedButtonSaveThumbAngle()
{
	m_pModel->CacheCurrentThumbAngle();
	m_pModel->UnloadThumbnail();
	m_pModel->LoadThumbnail();
	CAssetBrowserDialog::Instance()->GetAssetViewer().SelectAsset(m_pModel);
}
