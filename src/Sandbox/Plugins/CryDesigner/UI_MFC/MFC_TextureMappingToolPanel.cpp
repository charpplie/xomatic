#include "StdAfx.h"
#include "MFC_TextureMappingToolPanel.h"
#include "Tools/BrushDesignerTextureMappingTool.h"

namespace
{
	int g_nDesignerTextureMappingToolPanelId = 0;
	MFC_TextureMappingToolPanel* g_pDesignerTextureMappingToolPanel = NULL;
}

IMPLEMENT_DYNAMIC(MFC_TextureMappingToolPanel, CXTResizeDialog)
BEGIN_MESSAGE_MAP(MFC_TextureMappingToolPanel , CXTResizeDialog)
	ON_WM_DESTROY()
	ON_EN_CHANGE( IDC_TEXTURE_OFFSETX, OnValueChange )
	ON_EN_CHANGE( IDC_TEXTURE_OFFSETY, OnValueChange )
	ON_EN_CHANGE( IDC_TEXTURE_SCALEX, OnValueChange )
	ON_EN_CHANGE( IDC_TEXTURE_SCALEY, OnValueChange )
	ON_EN_CHANGE( IDC_TEXTURE_ROTATE, OnValueChange )
	ON_BN_CLICKED(IDC_TEXTURE_PICKSELECTED, OnBnClickedTexturePickselected)
	ON_BN_CLICKED(IDC_TEXTURE_FIT, OnBnClickedTextureFit)
	ON_BN_CLICKED(IDC_TEXTURE_RESET, OnBnClickedTextureReset)
	ON_BN_CLICKED(IDC_SELECT_MATID, OnBnClickedSelectMatid)
	ON_BN_CLICKED(IDC_ASSIGN_MATID, OnBnClickedAssignMatid)
	ON_BN_CLICKED(IDC_RELATIVE, OnRelative)
	ON_BN_CLICKED(IDC_ABSOLUTE, OnAbsolute)
END_MESSAGE_MAP()

ITextureMappingToolPanel* CreateTextureMappingPanel( CBrushDesignerTextureMappingTool* pTextureMappingTool, void* pData )
{
	if( !g_nDesignerTextureMappingToolPanelId )
	{
		g_pDesignerTextureMappingToolPanel = new MFC_TextureMappingToolPanel(pTextureMappingTool);
		g_nDesignerTextureMappingToolPanelId = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS,_T("Texture Mapping Tool"),g_pDesignerTextureMappingToolPanel,false,(int)pData);
	}
	return g_pDesignerTextureMappingToolPanel;
}

void MFC_TextureMappingToolPanel::DestroyPanel()
{
	if( g_nDesignerTextureMappingToolPanelId )
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS, g_nDesignerTextureMappingToolPanelId );
		g_pDesignerTextureMappingToolPanel = NULL;
		g_nDesignerTextureMappingToolPanelId = 0;
	}
}

MFC_TextureMappingToolPanel::MFC_TextureMappingToolPanel( CBrushDesignerTextureMappingTool* pTool, CWnd* pParent ) : m_pTextureMappingTool(pTool),
	CXTResizeDialog(MFC_TextureMappingToolPanel::IDD, pParent) 
{
	RESOURCEHANDLER_RECONSTRUCTOR;
	Create(IDD,pParent);
}

MFC_TextureMappingToolPanel::~MFC_TextureMappingToolPanel()
{
}

void MFC_TextureMappingToolPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control( pDX, IDC_ABSOLUTE, m_absoluteBtn );
	DDX_Control( pDX, IDC_RELATIVE, m_relativeBtn );
	DDX_Control( pDX, IDC_TEXTURE_PICKSELECTED, m_pickSelectedBtn );
}

BOOL MFC_TextureMappingToolPanel::OnInitDialog()
{
	BOOL bRes = __super::OnInitDialog();

	if( AfxGetApp()->GetProfileInt( "DesignerSetting", "Texture_AbsoluteBtn", 1 ) )
		m_absoluteBtn.SetCheck( BST_CHECKED );
	else
		m_absoluteBtn.SetCheck( BST_UNCHECKED );

	if( AfxGetApp()->GetProfileInt( "DesignerSetting", "Texture_RelativeBtn", 0 ) )
		m_relativeBtn.SetCheck( BST_CHECKED );
	else
		m_relativeBtn.SetCheck( BST_UNCHECKED );

	if( AfxGetApp()->GetProfileInt( "DesignerSetting", "Texture_PickBtn", 0 ) )
		m_pickSelectedBtn.SetCheck( BST_CHECKED );
	else
		m_pickSelectedBtn.SetCheck( BST_UNCHECKED );

	m_offset[0].Create( this,IDC_TEXTURE_OFFSETX ); 
	m_offset[1].Create( this,IDC_TEXTURE_OFFSETY );
	m_offset[0].SetValue(AfxGetApp()->GetProfileInt("DesignerSetting","Texture_OffsetX",0)/1000.0f);
	m_offset[1].SetValue(AfxGetApp()->GetProfileInt("DesignerSetting","Texture_OffsetY",0)/1000.0f);

	m_scale[0].Create( this,IDC_TEXTURE_SCALEX );
	m_scale[1].Create( this,IDC_TEXTURE_SCALEY );
	m_scale[0].SetValue(AfxGetApp()->GetProfileInt("DesignerSetting","Texture_ScaleX",1000)/1000.0f);
	m_scale[1].SetValue(AfxGetApp()->GetProfileInt("DesignerSetting","Texture_ScaleY",1000)/1000.0f);

	m_rotate.Create( this,IDC_TEXTURE_ROTATE );
	m_rotate.SetValue(AfxGetApp()->GetProfileInt("DesignerSetting","Texture_Rotate",0)/1000.0f);

	m_fitTiling[0].Create( this,IDC_TEXTURE_TILEX );
	m_fitTiling[1].Create( this,IDC_TEXTURE_TILEY );
	m_fitTiling[0].SetValue(AfxGetApp()->GetProfileInt("DesignerSetting","Texture_FitTilingX",1000)/1000.0f);
	m_fitTiling[1].SetValue(AfxGetApp()->GetProfileInt("DesignerSetting","Texture_FitTilingY",1000)/1000.0f);

	m_offset[0].SetInternalPrecision(3);
	m_offset[1].SetInternalPrecision(3);
	m_scale[0].SetInternalPrecision(3);
	m_scale[1].SetInternalPrecision(3);

	m_offset[0].SetStep(0.01);
	m_offset[1].SetStep(0.01);
	m_scale[0].SetStep(0.01);
	m_scale[1].SetStep(0.01);
	m_rotate.SetStep(2);

	const int nMinimum = -100000;
	const int nMaximum = 100000;
	m_offset[0].SetRange( nMinimum, nMaximum );
	m_offset[1].SetRange( nMinimum, nMaximum );
	m_scale[0].SetRange( nMinimum, nMaximum );
	m_scale[1].SetRange( nMinimum, nMaximum );
	m_rotate.SetRange( nMinimum, nMaximum );

	m_offset[0].EnableUndo( "Tex OffsetX Modified" );
	m_offset[1].EnableUndo( "Tex OffsetY Modified" );
	m_scale[0].EnableUndo( "Tex ScaleX Modified" );
	m_scale[1].EnableUndo( "Tex ScaleY Modified" );
	m_rotate.EnableUndo( "Tex Rotate Modified" );

	m_MatId.Create( this,IDC_MATID );
	m_MatId.SetInteger(true);
	m_MatId.SetRange(1,33);
	m_MatId.SetValue(AfxGetApp()->GetProfileInt( "DesignerSetting", "Texture_MatID",1 ));

	return bRes;
}

void MFC_TextureMappingToolPanel::OnDestroy()
{
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_OffsetX",m_offset[0].GetValue()*1000.0f );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_OffsetY",m_offset[1].GetValue()*1000.0f );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_ScaleX",m_scale[0].GetValue()*1000.0f );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_ScaleY",m_scale[1].GetValue()*1000.0f );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_Rotate",m_rotate.GetValue()*1000.0f );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_FitTilingX",m_fitTiling[0].GetValue()*1000.0f );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_FitTilingY",m_fitTiling[1].GetValue()*1000.0f );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_AbsoluteBtn",m_absoluteBtn.GetCheck() == BST_CHECKED ? 1 : 0 );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_RelativeBtn",m_relativeBtn.GetCheck() == BST_CHECKED ? 1 : 0 );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_PickBtn",m_pickSelectedBtn.GetCheck() == BST_CHECKED ? 1 : 0 );
	AfxGetApp()->WriteProfileInt( "DesignerSetting", "Texture_MatID", m_MatId.GetValue() );
}

void MFC_TextureMappingToolPanel::OnBnClickedTexturePickselected()
{
}

void MFC_TextureMappingToolPanel::OnBnClickedTextureFit()
{
	if( !m_pTextureMappingTool )
		return;

	float fTile_u = m_fitTiling[0].GetValue();
	float fTile_v = m_fitTiling[1].GetValue();

	CUndo undo("Fit TextureUV");
	m_pTextureMappingTool->RecordTextureMappingUndo("Fit TextureUV");

	m_pTextureMappingTool->FitTexture(fTile_u,fTile_v);
	m_pTextureMappingTool->UpdateBrush();
}

void MFC_TextureMappingToolPanel::OnBnClickedTextureReset()
{
	if( !m_pTextureMappingTool )
		return;
	BUtil::STexInfo texInfo;

	CUndo undo("Reset TextureUV");
	m_pTextureMappingTool->RecordTextureMappingUndo("Reset TextureUV");

	m_pTextureMappingTool->ApplyTextureInfo(texInfo,IsRelative());
	m_pTextureMappingTool->UpdateBrush();
	SetTexInfo(texInfo);
}

void MFC_TextureMappingToolPanel::OnBnClickedSelectMatid()
{
	if( m_pTextureMappingTool )
		m_pTextureMappingTool->SelectRegionsByMatID((int)m_MatId.GetValue()-1);
}

void MFC_TextureMappingToolPanel::OnBnClickedAssignMatid()
{
	if( m_pTextureMappingTool )
	{
		CUndo undo("Assign Material ID");
		m_pTextureMappingTool->RecordTextureMappingUndo("Assign Material ID");
		m_pTextureMappingTool->AssignMatID((int)m_MatId.GetValue()-1);
		m_pTextureMappingTool->UpdateBrush();
	}
}

void MFC_TextureMappingToolPanel::OnValueChange()
{
	CUndo undo("Changes of Texture Info");
	if( m_pTextureMappingTool )
		m_pTextureMappingTool->RecordTextureMappingUndo("Changes of Texture Info");

	ApplyChanges();

	if( m_pTextureMappingTool )
		m_pTextureMappingTool->UpdateBrush();

	if( IsRelative() )
		ResetControls();
}

void MFC_TextureMappingToolPanel::ResetControls()
{
	m_offset[0].SetValue(0);
	m_offset[1].SetValue(0);
	m_scale[0].SetValue(0);
	m_scale[1].SetValue(0);
	m_rotate.SetValue(0);
}

BUtil::STexInfo MFC_TextureMappingToolPanel::GetTexInfoFromControls() const
{
	BUtil::STexInfo texInfo;

	texInfo.shift[0] = m_offset[0].GetValue();
	texInfo.shift[1] = m_offset[1].GetValue();
	texInfo.scale[0] = m_scale[0].GetValue();
	texInfo.scale[1] = m_scale[1].GetValue();
	texInfo.rotate = m_rotate.GetValue();

	return texInfo;
}

void MFC_TextureMappingToolPanel::ApplyChanges()
{
	if( !m_pTextureMappingTool )
		return;

	BUtil::STexInfo texInfo;
	texInfo.shift[0] = m_offset[0].GetValue();
	texInfo.shift[1] = m_offset[1].GetValue();
	texInfo.scale[0] = m_scale[0].GetValue();
	texInfo.scale[1] = m_scale[1].GetValue();
	texInfo.rotate = m_rotate.GetValue();	

	m_pTextureMappingTool->ApplyTextureInfo(texInfo,IsRelative());
}

void MFC_TextureMappingToolPanel::SetTexInfo( const BUtil::STexInfo& texInfo )
{
	m_offset[0].SetValue(texInfo.shift[0]);
	m_offset[1].SetValue(texInfo.shift[1]);
	m_scale[0].SetValue(texInfo.scale[0]);
	m_scale[1].SetValue(texInfo.scale[1]);
	m_rotate.SetValue(texInfo.rotate);
}

void MFC_TextureMappingToolPanel::SetMatID( int nMatID )
{
	m_MatId.SetValue(nMatID+1);
}

void MFC_TextureMappingToolPanel::OnRelative()
{
	m_LastTexInfo = GetTexInfoFromControls();
	ResetControls();
}

void MFC_TextureMappingToolPanel::OnAbsolute()
{
	BUtil::STexInfo texInfo;
	if( m_pTextureMappingTool->GetTexInfoOfSelectedRegion(texInfo) )
		SetTexInfo(texInfo);
	else
		SetTexInfo(m_LastTexInfo);
}