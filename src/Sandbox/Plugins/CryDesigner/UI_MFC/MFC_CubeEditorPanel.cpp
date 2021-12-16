#include "StdAfx.h"
#include "MFC_CubeEditorPanel.h"
#include "Tools/BrushDesignerCubeEditor.h"
#include "Material/MaterialManager.h"

IMPLEMENT_DYNAMIC(MFC_CubeEditorPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_CubeEditorPanel, CXTResizeDialog)
	ON_WM_DESTROY()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_DESIGNER_CUBEEDITOR_ADD, OnBnClickedDesignerCubeeditorAdd)
	ON_BN_CLICKED(IDC_DESIGNER_CUBEEDITOR_REMOVE, OnBnClickedDesignerCubeeditorRemove)
	ON_BN_CLICKED(IDC_DESIGNER_CUBEEDITOR_PAINT, OnBnClickedDesignerCubeeditorPaint)
END_MESSAGE_MAP()

namespace 
{
	MFC_CubeEditorPanel* s_pCubeEditorPanel = NULL;
	int s_nCubeEditorPanelId = 0;
}

ICubeEditorPanel* CreateCubeEditorPanel( CBrushDesignerCubeEditor* pCubeEditor, void* pData )
{
	if( !s_pCubeEditorPanel )
	{
		s_pCubeEditorPanel = new MFC_CubeEditorPanel(pCubeEditor);
		s_nCubeEditorPanelId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS,"Box Attributes",s_pCubeEditorPanel,false,(int)pData);
	}
	return s_pCubeEditorPanel;
}

void MFC_CubeEditorPanel::DestroyPanel()
{
	if( s_nCubeEditorPanelId )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,s_nCubeEditorPanelId);
		s_pCubeEditorPanel = NULL;
		s_nCubeEditorPanelId = 0;
	}
}

MFC_CubeEditorPanel::MFC_CubeEditorPanel( CBrushDesignerCubeEditor* pCubeEditor, CWnd* pParent ) : 
	m_pCubeEditor(pCubeEditor)
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create( IDD,pParent );
}

BOOL MFC_CubeEditorPanel::OnInitDialog()
{
	__super::OnInitDialog();

	m_CubeSizeList.push_back((BrushFloat)0.125); 
	m_CubeSizeList.push_back((BrushFloat)0.25); 
	m_CubeSizeList.push_back((BrushFloat)0.5); 
	m_CubeSizeList.push_back((BrushFloat)1.0); 
	m_CubeSizeList.push_back((BrushFloat)2.0); 
	m_CubeSizeList.push_back((BrushFloat)4.0); 
	m_CubeSizeList.push_back((BrushFloat)8.0); 
	m_CubeSizeList.push_back((BrushFloat)16.0); 
	m_CubeSizeList.push_back((BrushFloat)32.0); 
	m_CubeSizeList.push_back((BrushFloat)64.0); 

	CComboBox* pCubeSizeCombo = (CComboBox*)GetDlgItem(IDC_DESIGNER_CUBESIZE_COMBO);
	for( int i = 0, iCount(m_CubeSizeList.size()); i < m_CubeSizeList.size(); ++i )
	{
		CString buff;
		buff.Format("%f",m_CubeSizeList[i]);
		pCubeSizeCombo->AddString(buff);
	}

	int nCubeSizeIndex = AfxGetApp()->GetProfileInt( "DesignerSetting", "CubeEditor_CubeSize_Index", 3 );
	pCubeSizeCombo->SetCurSel(nCubeSizeIndex);

	int nMergeSides = AfxGetApp()->GetProfileInt( "DesignerSetting", "CubeEditor_MergeSides", 0 );
	CButton* pMergeSidesBox = (CButton*)GetDlgItem(IDC_DESIGNER_CUBEEDITOR_MERGESIDES);
	if( nMergeSides == 1 )
		pMergeSidesBox->SetCheck(BST_CHECKED);
	else
		pMergeSidesBox->SetCheck(BST_UNCHECKED);

	UncheckAllButtons();
	int nEditMode = AfxGetApp()->GetProfileInt( "DesignerSetting", "CubeEditor_EditMode", 0 );
	if( nEditMode == CBrushDesignerCubeEditor::eEditorMode_Add )
		GetAddButton()->SetCheck(BST_CHECKED);
	else if( nEditMode == CBrushDesignerCubeEditor::eEditorMode_Remove )
		GetRemoveButton()->SetCheck(BST_CHECKED);
	else if( nEditMode == CBrushDesignerCubeEditor::eEditorMode_Paint )
		GetPaintButton()->SetCheck(BST_CHECKED);

	UpdateSubMaterialComboBox();

	if( !SetMaterial(GetIEditor()->GetMaterialManager()->GetCurrentMaterial()) )
	{
		int nMaterialID = AfxGetApp()->GetProfileInt( "DesignerSetting", "CubeEditor_MaterialID", 0 );
		SetSubMatID(nMaterialID);
	}

	return TRUE;
}

void MFC_CubeEditorPanel::OnDestroy()
{
	CComboBox* pCubeSizeCombo = (CComboBox*)GetDlgItem(IDC_DESIGNER_CUBESIZE_COMBO);
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "CubeEditor_CubeSize_Index", pCubeSizeCombo->GetCurSel() );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "CubeEditor_MergeSides", IsSidesMerged() ? 1 : 0 );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "CubeEditor_MaterialID", GetSubMatID()+1 );
	CBrushDesignerCubeEditor::EEditorMode editMode = CBrushDesignerCubeEditor::eEditorMode_Add;
	if( IsRemoveButtonChecked() )
		editMode = CBrushDesignerCubeEditor::eEditorMode_Remove;
	else if( IsPaintButtonChecked() )
		editMode = CBrushDesignerCubeEditor::eEditorMode_Paint;
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "CubeEditor_EditMode", (int)editMode );
}

BrushFloat MFC_CubeEditorPanel::GetCubeSize() const
{
	CComboBox* pCubeSizeCombo = (CComboBox*)GetDlgItem(IDC_DESIGNER_CUBESIZE_COMBO);
	int nCurSel = pCubeSizeCombo->GetCurSel();
	if( nCurSel != -1 )
		return m_CubeSizeList[nCurSel];
	return m_CubeSizeList[0];
}

bool MFC_CubeEditorPanel::IsSidesMerged() const
{
	CButton* pMergeSidesBox = (CButton*)GetDlgItem(IDC_DESIGNER_CUBEEDITOR_MERGESIDES);
	return pMergeSidesBox->GetCheck() == BST_CHECKED;
}

void MFC_CubeEditorPanel::UncheckAllButtons()
{
	GetRemoveButton()->SetCheck(BST_UNCHECKED);
	GetAddButton()->SetCheck(BST_UNCHECKED);
	GetPaintButton()->SetCheck(BST_UNCHECKED);
}

void MFC_CubeEditorPanel::OnBnClickedDesignerCubeeditorAdd()
{
	UncheckAllButtons();
	GetAddButton()->SetCheck(BST_CHECKED);
}

void MFC_CubeEditorPanel::OnBnClickedDesignerCubeeditorRemove()
{
	UncheckAllButtons();
	GetRemoveButton()->SetCheck(BST_CHECKED);
}

void MFC_CubeEditorPanel::OnBnClickedDesignerCubeeditorPaint()
{
	UncheckAllButtons();
	GetPaintButton()->SetCheck(BST_CHECKED);
}

CButton* MFC_CubeEditorPanel::GetAddButton() const
{
	return (CButton*)GetDlgItem(IDC_DESIGNER_CUBEEDITOR_ADD);
}

CButton* MFC_CubeEditorPanel::GetRemoveButton() const
{
	return (CButton*)GetDlgItem(IDC_DESIGNER_CUBEEDITOR_REMOVE);
}

CButton* MFC_CubeEditorPanel::GetPaintButton() const
{
	return (CButton*)GetDlgItem(IDC_DESIGNER_CUBEEDITOR_PAINT);
}

bool MFC_CubeEditorPanel::IsAddButtonChecked() const
{
	return GetAddButton()->GetCheck() == BST_CHECKED;
}

bool MFC_CubeEditorPanel::IsRemoveButtonChecked() const
{
	return GetRemoveButton()->GetCheck() == BST_CHECKED;
}

bool MFC_CubeEditorPanel::IsPaintButtonChecked() const
{
	return GetPaintButton()->GetCheck() == BST_CHECKED;
}

int MFC_CubeEditorPanel::GetSubMatID() const
{
	CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_DESIGNER_CUBEEDIT_SUBMATID_COMBO);
	CString matIDStr;
	pComboBox->GetWindowText(matIDStr);
	int nMatID = atoi(matIDStr)-1;
	return nMatID == -1 ? 0 : nMatID;
}

void MFC_CubeEditorPanel::SetSubMatID( int nID ) const 
{
	CComboBox* pSubMatIDComboBox = (CComboBox*)GetDlgItem(IDC_DESIGNER_CUBEEDIT_SUBMATID_COMBO);

	if( m_pCubeEditor->GetBaseObject() )
	{
		CMaterial* pMaterial = m_pCubeEditor->GetBaseObject()->GetMaterial();
		if( !pMaterial || nID > pMaterial->GetSubMaterialCount() || nID < 1 )
			pSubMatIDComboBox->SetCurSel(0);
		else
			pSubMatIDComboBox->SetCurSel(nID-1);
	}
	else
	{
		pSubMatIDComboBox->SetCurSel(0);
	}
}

bool MFC_CubeEditorPanel::SetMaterial( CMaterial* pMaterial )
{
	if( pMaterial == NULL || m_pCubeEditor->GetBaseObject() == NULL )
		return false;
	CMaterial* pParentMat = pMaterial->GetParent();
	if( pParentMat && pParentMat == m_pCubeEditor->GetBaseObject()->GetMaterial() )
	{
		for( int i = 0, nSubMaterialCount(pParentMat->GetSubMaterialCount()); i < nSubMaterialCount; ++i )
		{
			if( pMaterial == pParentMat->GetSubMaterial(i) )
			{
				SetSubMatID(i+1);
				return true;
			}
		}
	}
	return false;
}

void MFC_CubeEditorPanel::SelectPrevBrush()
{
	CComboBox* pCubeSizeCombo = (CComboBox*)GetDlgItem(IDC_DESIGNER_CUBESIZE_COMBO);
	int nCurSel = pCubeSizeCombo->GetCurSel();
	if( nCurSel < pCubeSizeCombo->GetCount()-1 )
		pCubeSizeCombo->SetCurSel(nCurSel+1);
}

void MFC_CubeEditorPanel::SelectNextBrush()
{
	CComboBox* pCubeSizeCombo = (CComboBox*)GetDlgItem(IDC_DESIGNER_CUBESIZE_COMBO);
	int nCurSel = pCubeSizeCombo->GetCurSel();
	if( nCurSel > 0 )
		pCubeSizeCombo->SetCurSel(nCurSel-1);
}

void MFC_CubeEditorPanel::UpdateSubMaterialComboBox()
{
	CComboBox* pSubMatIDComboBox = (CComboBox*)GetDlgItem(IDC_DESIGNER_CUBEEDIT_SUBMATID_COMBO);
	int nCurSel = pSubMatIDComboBox->GetCurSel();
	pSubMatIDComboBox->ResetContent();
	if( m_pCubeEditor->GetBaseObject() == NULL )
		return;
	CMaterial* pMaterial = m_pCubeEditor->GetBaseObject()->GetMaterial();
	if( pMaterial == NULL )
		return;
	for( int i = 1, iSubMatID(pMaterial->GetSubMaterialCount()); i <= iSubMatID; ++i )
	{
		CMaterial* pSubMaterial = pMaterial->GetSubMaterial(i-1);
		if( pSubMaterial == NULL )
			continue;
		CString matIDStr;
		matIDStr.Format("%d.%s",i,pSubMaterial->GetFullName());
		pSubMatIDComboBox->AddString(matIDStr);
	}
	if( nCurSel != -1 )
		SetSubMatID(nCurSel+1);
	else
		SetSubMatID(1);
}