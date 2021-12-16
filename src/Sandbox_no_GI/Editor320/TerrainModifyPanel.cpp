// TerrainModifyPanel.cpp : implementation file
//

#include "stdafx.h"
#include "TerrainModifyPanel.h"
#include "TerrainModifyTool.h"

/////////////////////////////////////////////////////////////////////////////
// CTerrainModifyPanel dialog


bool CTerrainModifyPanel::m_isSyncRadius = false;


CTerrainModifyPanel::CTerrainModifyPanel(CTerrainModifyTool *tool,CWnd* pParent /*=NULL*/)
	: CXTResizeDialog(CTerrainModifyPanel::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTerrainModifyPanel)
	//}}AFX_DATA_INIT

	Create( IDD,pParent );

	assert( tool != 0 );
	m_tool = tool;
}

void CTerrainModifyPanel::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTerrainModifyPanel)
	DDX_Control(pDX, IDC_BRUSH_TYPE, m_brushType);
	DDX_Control(pDX, IDC_NOISE_FREQ_SLIDER, m_noiseFreqSlider);
	DDX_Control(pDX, IDC_NOISE_SCALE_SLIDER, m_noiseScaleSlider);
	DDX_Control(pDX, IDC_BRUSH_HARDNESS_SLIDER, m_hardnessSlider);
	DDX_Control(pDX, IDC_BRUSH_HEIGHT_SLIDER, m_heightSlider);
	DDX_Control(pDX, IDC_BRUSH_RADIUS_SLIDER, m_radiusSlider);
	DDX_Control(pDX, IDC_BRUSH_RADIUS_SLIDER2, m_radiusSlider2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTerrainModifyPanel, CXTResizeDialog)
	//{{AFX_MSG_MAP(CTerrainModifyPanel)
	ON_WM_HSCROLL()
	ON_EN_UPDATE(IDC_BRUSH_RADIUS, OnUpdateNumbers)
	ON_EN_UPDATE(IDC_BRUSH_RADIUS2, OnUpdateNumbers)
	ON_BN_CLICKED(IDC_BRUSH_NOISE, OnBrushNoise)
	ON_CBN_SELENDOK(IDC_BRUSH_TYPE, OnSelendokBrushType)
	ON_EN_UPDATE(IDC_BRUSH_HEIGHT, OnUpdateNumbers)
	ON_EN_UPDATE(IDC_BRUSH_HARDNESS, OnUpdateNumbers)
	ON_EN_UPDATE(IDC_NOISE_SCALE, OnUpdateNumbers)
	ON_EN_UPDATE(IDC_NOISE_FREQ, OnUpdateNumbers)
	ON_BN_CLICKED(IDC_REPOSITION_OBJECTS, OnRepositionObjects)
	ON_BN_CLICKED(IDC_REPOSITION_VEGETATION, OnRepositionVegetation)
	ON_BN_CLICKED(IDC_SYNC_RADIUS, OnSyncRadius)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTerrainModifyPanel message handlers

BOOL CTerrainModifyPanel::OnInitDialog() 
{
	CXTResizeDialog::OnInitDialog();
	
	m_brushRadius.Create( this,IDC_BRUSH_RADIUS );
	m_brushRadius.SetRange( 1,200 );
	
	m_brushRadius2.Create( this,IDC_BRUSH_RADIUS2 );
	m_brushRadius2.SetRange( 0,200 );
	
	m_brushHeight.Create( this,IDC_BRUSH_HEIGHT );
	m_brushHeight.SetRange( 0,255 );

	m_brushHardness.Create( this,IDC_BRUSH_HARDNESS );
	m_brushHardness.SetRange( 0,1 );

	m_noiseScale.Create( this,IDC_NOISE_SCALE );
	m_noiseScale.SetRange( 0,100 );

	m_noiseFreq.Create( this,IDC_NOISE_FREQ );
	m_noiseFreq.SetRange( 0,100 );
	
	float fMax=200;
	m_radiusSlider.SetRange( 1,100 * log10(fMax) );
	m_radiusSlider2.SetRange( 1,100 * log10(fMax) );

	m_heightSlider.SetRange( 0,255 );
	m_hardnessSlider.SetRange( 0,100 );

	m_noiseScaleSlider.SetRange( 0,100 );
	m_noiseFreqSlider.SetRange( 0,100 );

	m_brushType.AddString( "Flatten" );
	m_brushType.AddString( "Smooth" );
	m_brushType.AddString( "Rise/Lower" );
	m_brushType.AddString( "Pick Height" );

	SetResize( IDC_NOISE_FREQ_SLIDER,SZ_HORRESIZE(1) );
	SetResize( IDC_NOISE_SCALE_SLIDER,SZ_HORRESIZE(1) );
	SetResize( IDC_BRUSH_HARDNESS_SLIDER,SZ_HORRESIZE(1) );
	SetResize( IDC_BRUSH_HEIGHT_SLIDER,SZ_HORRESIZE(1) );
	SetResize( IDC_BRUSH_RADIUS_SLIDER,SZ_HORRESIZE(1) );
	SetResize( IDC_BRUSH_RADIUS_SLIDER2,SZ_HORRESIZE(1) );

	SetResize( IDC_STATIC1,SZ_HORRESIZE(1) );
	SetResize( IDC_STATIC2,SZ_HORRESIZE(1) );

	CheckDlgButton(IDC_SYNC_RADIUS, m_isSyncRadius);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::SetBrush( CTerrainBrush * pBrush )
{
	if (pBrush->type != eBrushPickHeight)
	{
		EnableRadius(TRUE);
		EnableHardness(TRUE);
	}
	m_brushHeight.SetRange( pBrush->heightRange.x, pBrush->heightRange.y );
	m_heightSlider.SetRange( pBrush->heightRange.x, pBrush->heightRange.y );
	if (pBrush->type == eBrushFlatten)
	{
		m_brushType.SetCurSel(0);
		EnableRadiusInner(TRUE);
		EnableHeight(TRUE);
	}
	else if (pBrush->type == eBrushSmooth)
	{
		m_brushType.SetCurSel(1);
		pBrush->bNoise = false;
		EnableRadiusInner(FALSE);
		EnableHeight(FALSE);
	}
	else if (pBrush->type == eBrushRiseLower)
	{
		m_brushType.SetCurSel(2);
		EnableRadiusInner(TRUE);
		EnableHeight(TRUE);
	}
	else if (pBrush->type == eBrushPickHeight)
	{
		m_brushType.SetCurSel(3);
		EnableRadius(FALSE);
		EnableRadiusInner(FALSE);
		EnableHardness(FALSE);
		EnableHeight(TRUE);
	}
	m_brushRadius.SetValue( pBrush->radius );
	m_brushRadius2.SetValue( pBrush->radiusInside );
	m_brushHeight.SetValue( pBrush->height );
	m_brushHardness.SetValue( pBrush->hardness );
	m_noiseFreq.SetValue( pBrush->noiseFreq );
	m_noiseScale.SetValue( pBrush->noiseScale );

	m_radiusSlider.SetPos( log10(max(pBrush->radius,0.001f))*100.0f );
	m_radiusSlider2.SetPos( log10(max(pBrush->radiusInside,0.001f))*100.0f );
	m_heightSlider.SetPos( pBrush->height );
	m_hardnessSlider.SetPos( pBrush->hardness*100.0f );
	m_noiseScaleSlider.SetPos( pBrush->noiseScale );
	m_noiseFreqSlider.SetPos( pBrush->noiseFreq );

	m_radiusSlider.SetSelection( 0,m_radiusSlider.GetPos() );
	m_radiusSlider2.SetSelection( 0,m_radiusSlider2.GetPos() );
	m_heightSlider.SetSelection( 0,m_heightSlider.GetPos() );
	m_hardnessSlider.SetSelection( 0,m_hardnessSlider.GetPos() );

	m_noiseScaleSlider.EnableWindow(pBrush->bNoise);
	m_noiseFreqSlider.EnableWindow(pBrush->bNoise);
	m_noiseScale.EnableWindow(pBrush->bNoise);
	m_noiseFreq.EnableWindow(pBrush->bNoise);

	CheckDlgButton( IDC_BRUSH_NOISE,(pBrush->bNoise)?BST_CHECKED:BST_UNCHECKED );
	CheckDlgButton( IDC_REPOSITION_OBJECTS,(pBrush->bRepositionObjects)?BST_CHECKED:BST_UNCHECKED );
	CheckDlgButton( IDC_REPOSITION_VEGETATION,(pBrush->bRepositionVegetation)?BST_CHECKED:BST_UNCHECKED );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::OnUpdateNumbers() 
{
	CTerrainBrush br;
	m_tool->GetCurBrushParams(br);
	float prevRadius = br.radius;
	float prevRadiusInside = br.radiusInside;
	br.radius = m_brushRadius.GetValue();
	br.radiusInside = m_brushRadius2.GetValue();
	if (br.radius < br.radiusInside)
	{
		if (prevRadiusInside != br.radiusInside) // Check if changing inside radius.
			br.radius = br.radiusInside;
		else
			br.radiusInside = br.radius; // Changing outside radius;
	}
	br.height = m_brushHeight.GetValue();
	br.hardness = m_brushHardness.GetValue();
	br.noiseFreq = m_noiseFreq.GetValue();
	br.noiseScale = m_noiseScale.GetValue();
	m_tool->SetCurBrushParams(br, m_isSyncRadius);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::OnBrushNoise() 
{
	BOOL noise = IsDlgButtonChecked(IDC_BRUSH_NOISE);
	CTerrainBrush br;
	m_tool->GetCurBrushParams(br);
	br.bNoise = (noise)?true:false;
	m_tool->SetCurBrushParams(br);
}


//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::OnSyncRadius() 
{
	m_isSyncRadius = IsDlgButtonChecked(IDC_SYNC_RADIUS);
}


//////////////////////////////////////////////////////////////////////////
void CTerrainModifyPanel::OnHScroll( UINT nSBCode,UINT nPos,CScrollBar* pScrollBar )
{
	CSliderCtrl *pSliderCtrl = (CSliderCtrl*)pScrollBar;
	CTerrainBrush br;
	m_tool->GetCurBrushParams(br);
	if (pSliderCtrl == &m_hardnessSlider)
	{
		br.hardness = (float)m_hardnessSlider.GetPos()/100.0f;
	}
	else if (pSliderCtrl == &m_heightSlider)
	{
		br.height = m_heightSlider.GetPos();
	}
	else if (pSliderCtrl == &m_radiusSlider)
	{
		br.radius = pow(10.0f,m_radiusSlider.GetPos()/100.0f);
		if (br.radius < br.radiusInside)
			br.radiusInside = br.radius;
	}
	else if (pSliderCtrl == &m_radiusSlider2)
	{
		br.radiusInside = pow(10.0f,m_radiusSlider2.GetPos()/100.0f);
		if (br.radius < br.radiusInside)
			br.radius = br.radiusInside;
	}
	else if (pSliderCtrl == &m_noiseScaleSlider)
	{
		br.noiseScale = m_noiseScaleSlider.GetPos();
	}
	else if (pSliderCtrl == &m_noiseFreqSlider)
	{
		br.noiseFreq = m_noiseFreqSlider.GetPos();
	}

	m_tool->SetCurBrushParams(br, m_isSyncRadius);
}

void CTerrainModifyPanel::OnSelendokBrushType() 
{
	int sel = m_brushType.GetCurSel();
	if (sel != LB_ERR)
	{
		switch (sel)
		{
		case 0:
			m_tool->SetCurBrushType(eBrushFlatten);
			break;
		case 1:
			m_tool->SetCurBrushType(eBrushSmooth);
			break;
		case 2:
			m_tool->SetCurBrushType(eBrushRiseLower);
			break;
		case 3:
			m_tool->SetCurBrushType(eBrushPickHeight);
			break;
		}
	}
}

void CTerrainModifyPanel::OnRepositionObjects() 
{
	BOOL noise = IsDlgButtonChecked(IDC_REPOSITION_OBJECTS);
	CTerrainBrush br;
	m_tool->GetCurBrushParams(br);
	br.bRepositionObjects = (noise)?true:false;
	m_tool->SetCurBrushParams(br);
}

void CTerrainModifyPanel::OnRepositionVegetation() 
{
	BOOL noise = IsDlgButtonChecked(IDC_REPOSITION_VEGETATION);
	CTerrainBrush br;
	m_tool->GetCurBrushParams(br);
	br.bRepositionVegetation = (noise)?true:false;
	m_tool->SetCurBrushParams(br);
}


void CTerrainModifyPanel::EnableRadius(BOOL isEnable)
{
	m_radiusSlider.EnableWindow(isEnable);
	m_brushRadius.EnableWindow(isEnable);
}

void CTerrainModifyPanel::EnableRadiusInner(BOOL isEnable)
{
	m_radiusSlider2.EnableWindow(isEnable);
	m_brushRadius2.EnableWindow(isEnable);
}

void CTerrainModifyPanel::EnableHardness(BOOL isEnable)
{
	m_hardnessSlider.EnableWindow(isEnable);
	m_brushHardness.EnableWindow(isEnable);
}

void CTerrainModifyPanel::EnableHeight(BOOL isEnable)
{
	m_heightSlider.EnableWindow(isEnable);
	m_brushHeight.EnableWindow(isEnable);
}