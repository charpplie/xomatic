// CharPanel_Character.cpp : implementation file
//

#include "stdafx.h"
#include "CharPanel_Character.h"
#include "..\MaterialSelectorDialog.h"
#include "ModelViewportCE.h"
#include "..\Material\MaterialManager.h"
#include "CharacterEditor.h"

// CCharacterPropertiesDlg dialog

IMPLEMENT_DYNAMIC(CCharacterPropertiesDlg, CDialog)

CCharacterPropertiesDlg::CCharacterPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCharacterPropertiesDlg::IDD, pParent)
{
	m_pModelViewportCE = 0;

	pCharacterChangeListener = 0;
}

CCharacterPropertiesDlg::~CCharacterPropertiesDlg()
{
}

void CCharacterPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_MATERIAL_NAME, m_materialEdit);
}

BOOL CCharacterPropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetOverrideMaterial();

	return TRUE;
}

void CCharacterPropertiesDlg::OnOK()
{
	// Don't call the base class implementation - somehow ok messages are being generated
	// in response to command messages - this would close the window unless we stop it here.
}

//////////////////////////////////////////////////////////////////////////
void CCharacterPropertiesDlg::SetOverrideMaterial()
{
	IMaterial* pOverrideMaterial = 0;
	if (m_pModelViewportCE != 0)
	{
		ICharacterInstance* pCharacterInstance = m_pModelViewportCE->GetCharacterBase();
		if (pCharacterInstance != 0)
			pOverrideMaterial = pCharacterInstance->GetMaterialOverride();
	}
	if (pOverrideMaterial != 0)
		m_materialEdit.SetWindowText(pOverrideMaterial->GetName());
	else
		m_materialEdit.SetWindowText("");
	
}

void CCharacterPropertiesDlg::SetMaterial(CMaterial* pMaterial)
{
	_smart_ptr<IMaterial> pIMaterial = 0;
	if (pMaterial)
		pIMaterial = pMaterial->GetMatInfo();
	if(m_pModelViewportCE->GetCharacterBase())
		m_pModelViewportCE->GetCharacterBase()->SetMaterial(pIMaterial);
	this->SendOnCharacterChanged();

	if (pIMaterial != 0)
		m_materialEdit.SetWindowText(pIMaterial->GetName());
	else
		m_materialEdit.SetWindowText("");
}

BEGIN_MESSAGE_MAP(CCharacterPropertiesDlg, CDialog)
	ON_BN_CLICKED(IDC_DEFAULT_MTL, OnBnClickedDefaultMtl)
	ON_BN_CLICKED(IDC_APPLY_MATERIAL, OnBnClickedApplyMaterial)
	ON_BN_CLICKED(IDC_OVERRIDE_MATERIAL, OnBnClickedOverrideMaterial)
	ON_BN_CLICKED(IDC_MATERIAL_SELECT, OnBnClickedSelectMaterial)
END_MESSAGE_MAP()

// CCharacterPropertiesDlg message handlers

void CCharacterPropertiesDlg::OnBnClickedSelectMaterial()
{
	// Try to find the material used by the displayed asset.
	IMaterial* pMaterial = 0;
	if (m_pModelViewportCE->GetCharacterBase() && m_pModelViewportCE->GetCharacterBase()->GetMaterial())
		pMaterial = m_pModelViewportCE->GetCharacterBase()->GetMaterial();
	else if (m_pModelViewportCE->GetStaticObject() && m_pModelViewportCE->GetStaticObject()->GetMaterial())
		pMaterial = m_pModelViewportCE->GetStaticObject()->GetMaterial();

	// Open the material editor, showing the material if one was found.
	IDataBaseItem *pItem = 0;
	if (pMaterial)
	{
		CString name = pMaterial->GetName();
		pItem = GetIEditor()->GetMaterialManager()->FindItemByName(name);
	}
	// If we could not find the material, report the problem.
	else
	{
		this->MessageBox("Unable to find material to show - opening material editor with no selection.", "No Material", MB_ICONEXCLAMATION | MB_OK);
	}

	GetIEditor()->OpenDataBaseLibrary( EDB_TYPE_MATERIAL,pItem );
}
//////////////////////////////////////////////////////////////////////////
void CCharacterPropertiesDlg::OnBnClickedOverrideMaterial()
{
	IMaterial* pOverrideMaterial = 0;
	if (m_pModelViewportCE != 0)
	{
		ICharacterInstance* pCharacterInstance = m_pModelViewportCE->GetCharacterBase();
		if (pCharacterInstance != 0)
			pOverrideMaterial = pCharacterInstance->GetMaterialOverride();
	}

	if(m_pModelViewportCE->GetCharacterBase())
		m_pModelViewportCE->GetCharacterBase()->SetMaterial(pOverrideMaterial);
	this->SendOnCharacterChanged();

	if (pOverrideMaterial != 0)
		m_materialEdit.SetWindowText(pOverrideMaterial->GetName());
	else
		m_materialEdit.SetWindowText("");
}

void CCharacterPropertiesDlg::OnBnClickedDefaultMtl()
{
	CMaterial* defaultMaterial = m_pModelViewportCE->GetDefaultMaterial();
	if(!defaultMaterial)
		return;

	IMaterial* pIMaterial = defaultMaterial->GetMatInfo();
	m_pModelViewportCE->GetCharacterBase()->SetMaterial(pIMaterial);
	
	if (pIMaterial != 0)
		m_materialEdit.SetWindowText(pIMaterial->GetName());
	else
		m_materialEdit.SetWindowText("");

	this->SendOnCharacterChanged();
}

//////////////////////////////////////////////////////////////////////////
void CCharacterPropertiesDlg::OnBnClickedApplyMaterial()
{
	_smart_ptr<CMaterial> pMaterial = GetIEditor()->GetMaterialManager()->GetCurrentMaterial();
	_smart_ptr<IMaterial> pIMaterial = 0;
	if (pMaterial)
		pIMaterial = pMaterial->GetMatInfo();
	m_pModelViewportCE->GetCharacterBase()->SetMaterial(pIMaterial);

	this->SendOnCharacterChanged();

	if (pIMaterial != 0)
		m_materialEdit.SetWindowText(pIMaterial->GetName());
	else
		m_materialEdit.SetWindowText("");
}

void CCharacterPropertiesDlg::SetCharacterChangeListener(ICharacterChangeListener* pListener)
{
	this->pCharacterChangeListener = pListener;
}

void CCharacterPropertiesDlg::SendOnCharacterChanged()
{
	if (this->pCharacterChangeListener)
		this->pCharacterChangeListener->OnCharacterChanged();
}
