//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "MaterialSelectorDialog.h"
#include "Material/Material.h"
#include "Material/MaterialManager.h"


// CMaterialSelectorDialog dialog

IMPLEMENT_DYNAMIC(CMaterialSelectorDialog, CDialog)
CMaterialSelectorDialog::CMaterialSelectorDialog(CWnd* pParent /*=NULL*/)
: CXTResizeDialog(CMaterialSelectorDialog::IDD, pParent),
	browserListener(sMaterialName, m_materialNameEdit, m_matrialImageListCtrl)
{
}

CMaterialSelectorDialog::~CMaterialSelectorDialog()
{
}

void CMaterialSelectorDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_MATERIAL_NAME, m_materialNameEdit);
	DDX_Control(pDX, IDC_MATERIAL_BROWSER_PARENT, m_materialBrowserParent);
	DDX_Control(pDX, IDC_MATERIAL_IMAGE_PARENT, m_materialImageListParent);
}


BEGIN_MESSAGE_MAP(CMaterialSelectorDialog, CXTResizeDialog)
	ON_COMMAND(IDCANCEL, OnCancel)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDOK, &CMaterialSelectorDialog::OnBnClickedOk)
END_MESSAGE_MAP()


// CMaterialSelectorDialog message handlers

BOOL CMaterialSelectorDialog::OnInitDialog()
{
	BOOL bReturnValue = __super::OnInitDialog();

	// Listen to the material name.
	this->m_materialBrowserCtrl.SetListener(&this->browserListener);

	// Create the material browser control.
	CRect browserRect;
	m_materialBrowserParent.GetClientRect(browserRect);
	m_materialBrowserCtrl.Create(browserRect, &m_materialBrowserParent, AFX_IDW_PANE_FIRST);
	m_materialBrowserCtrl.ShowWindow(SW_SHOW);

	// Create the material image list control.
	CRect imageListRect;
	m_materialImageListParent.GetClientRect(imageListRect);
	m_matrialImageListCtrl.Create(ILC_STYLE_HORZ|WS_CHILD|WS_VISIBLE, imageListRect, &m_materialImageListParent, AFX_IDW_PANE_FIRST);
	m_materialBrowserCtrl.SetImageListCtrl(&m_matrialImageListCtrl);

	// Fill the material browser with items.
	m_materialBrowserCtrl.ReloadItems(CMaterialBrowserCtrl::VIEW_ALL);

	// Select the appropriate item in the browser.
	IDataBaseItem *pItem = GetIEditor()->GetMaterialManager()->FindItemByName(this->sMaterialName.GetString());
	m_materialBrowserCtrl.SelectItem(pItem);

	// setup resizing of controls
	SetResize( IDC_MATERIAL_BROWSER_PARENT, 0, 0, 1, 1 );
	SetResize( IDC_MATERIAL_IMAGE_PARENT, 0, 1, 1, 1 );
	SetResize( IDC_MATERIAL_NAME, 0, 1, 1, 1 );
	SetResize( IDOK, 0, 1, 0, 1 );
	SetResize( IDCANCEL, 0, 1, 0, 1 );

	// load saved window position
	LoadWindowPos( this, "MaterialSelectorDialog", "WindowPersistence" );
	// center window
	CenterWindow();

	// resize the material browser, needs so that it shows the tree and toolbar in place
	CRect rc;
	m_materialBrowserParent.GetClientRect( &rc );
	m_materialBrowserCtrl.MoveWindow( rc );

	return bReturnValue;
}

void CMaterialSelectorDialog::OnCancel()
{
	// save the window pos/size to registry
	SaveWindowPos( this, "MaterialSelectorDialog", "WindowPersistence" );
	__super::OnCancel();
}

void CMaterialSelectorDialog::OnBnClickedOk()
{
	// save the window pos/size to registry
	SaveWindowPos( this, "MaterialSelectorDialog", "WindowPersistence" );
	__super::OnOK();
}

void CMaterialSelectorDialog::SetMaterialName(const char* szMaterialName)
{
	this->sMaterialName = szMaterialName;
}

const char* CMaterialSelectorDialog::GetMaterialName()
{
	return this->sMaterialName.GetString();
}

CMaterialSelectorDialog::CMaterialBrowserListener::CMaterialBrowserListener(CString& sMaterialName, CEdit& materialNameEdit, CMaterialImageListCtrl& matrialImageListCtrl)
: sMaterialName(sMaterialName),
	materialNameEdit(materialNameEdit),
	matrialImageListCtrl(matrialImageListCtrl)
{
}

void CMaterialSelectorDialog::CMaterialBrowserListener::OnBrowserSelectItem(IDataBaseItem* pItem, bool bForce)
{
	CMaterial* pMaterial = (CMaterial*)pItem;
	if (pMaterial != 0)
		this->sMaterialName = pMaterial->GetName();
	else
		this->sMaterialName == "";
	this->materialNameEdit.SetWindowText(this->sMaterialName.GetString());
	this->matrialImageListCtrl.SelectMaterial(pMaterial);
}

void CMaterialSelectorDialog::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	if(m_materialBrowserParent.m_hWnd)
	{
		CRect rc;
		// resize the child controls within the parent controls
		m_materialBrowserParent.GetClientRect( &rc );
		m_materialBrowserCtrl.MoveWindow( rc );
		m_materialImageListParent.GetClientRect( &rc );
		m_matrialImageListCtrl.MoveWindow( rc );
	}
}
