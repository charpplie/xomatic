////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserPreviewModelDlg.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//	Description: Implementation of AssetBrowserPreviewModelDlg.h
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AssetBrowserPreviewModelDlg.h"
#include "AssetModelItem.h"
#include "Asset Browser/AssetBrowserDialog.h"
#include "AssetBrowserPreviewModelDlgFooter.h"


IMPLEMENT_DYNAMIC(CAssetBrowserPreviewModelDlg, CDialog)

CAssetBrowserPreviewModelDlg::CAssetBrowserPreviewModelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAssetBrowserPreviewModelDlg::IDD, pParent)
{
	m_pModel = NULL;
	m_pFooter = NULL;
}

CAssetBrowserPreviewModelDlg::~CAssetBrowserPreviewModelDlg()
{
	m_pModel = NULL;
	m_pFooter = NULL;
}

void CAssetBrowserPreviewModelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ASSET_BROWSE_MODEL_LIGHTING_COMBO, m_lightingCB);
	DDX_Control(pDX, IDC_BUTTON_FULLSCREEN, m_fullscreenBtn);
}

BEGIN_MESSAGE_MAP(CAssetBrowserPreviewModelDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_WIREFRAME, OnBnClickedButtonWireframe)
	ON_BN_CLICKED(IDC_CHECK_PHYSICS, OnBnClickedButtonPhysics)
	ON_BN_CLICKED(IDC_CHECK_NORMALS, OnBnClickedButtonNormals)
	ON_BN_CLICKED(IDC_BUTTON_RESET_VIEW, OnBnClickedButtonResetView)
	ON_BN_CLICKED(IDC_BUTTON_FULLSCREEN, OnBnClickedButtonFullscreen)
	ON_CBN_SELCHANGE(IDC_ASSET_BROWSE_MODEL_LIGHTING_COMBO, OnLightingChanged)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_THUMB_ANGLE, &CAssetBrowserPreviewModelDlg::OnBnClickedButtonSaveThumbAngle)
END_MESSAGE_MAP()

BOOL CAssetBrowserPreviewModelDlg::OnInitDialog()
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

// CAssetBrowserPreviewModelDlg message handlers
void CAssetBrowserPreviewModelDlg::Init()
{
	CheckDlgButton(IDC_CHECK_WIREFRAME, CAssetModelItem::s_bWireframe);
	CheckDlgButton(IDC_CHECK_PHYSICS, CAssetModelItem::s_bPhysics);
	CheckDlgButton(IDC_CHECK_NORMALS, CAssetModelItem::s_bNormals);
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonWireframe()
{
	CAssetModelItem::s_bWireframe = IsDlgButtonChecked(IDC_CHECK_WIREFRAME) == BST_CHECKED;

	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonPhysics()
{
	CAssetModelItem::s_bPhysics = IsDlgButtonChecked(IDC_CHECK_PHYSICS) == BST_CHECKED;

	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonNormals()
{
	CAssetModelItem::s_bNormals = IsDlgButtonChecked(IDC_CHECK_NORMALS) == BST_CHECKED;

	GetParent()->RedrawWindow();
}

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonResetView()
{
	if (m_pModel)
	{
		CheckDlgButton(IDC_CHECK_WIREFRAME, (CAssetModelItem::s_bWireframe = false));
		CheckDlgButton(IDC_CHECK_PHYSICS, (CAssetModelItem::s_bPhysics = false));
		CheckDlgButton(IDC_CHECK_NORMALS, (CAssetModelItem::s_bNormals = false));
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

void CAssetBrowserPreviewModelDlg::OnLightingChanged()
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

CXTPDockingPaneManager* CAssetBrowserPreviewModelDlg::GetDockingManager()
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

CXTPDockingPane* CAssetBrowserPreviewModelDlg::GetDockPane(CXTPDockingPaneManager* pDockingManager)
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

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonFullscreen()
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

void CAssetBrowserPreviewModelDlg::DockViewPane()
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

void CAssetBrowserPreviewModelDlg::OnBnClickedButtonSaveThumbAngle()
{
	m_pModel->CacheCurrentThumbAngle();
	m_pModel->UnloadThumbnail();
	m_pModel->LoadThumbnail();
	CAssetBrowserDialog::Instance()->GetAssetViewer().SelectAsset(m_pModel);
}
