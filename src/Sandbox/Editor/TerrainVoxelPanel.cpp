// TerrainVoxelPanel.cpp : implementation file
//

#include "stdafx.h"
#include "TerrainVoxelPanel.h"
#include "TerrainVoxelTool.h"
#include ".\Terrain\SurfaceType.h"
#include "VegetationMap.h"

#include "CryEditDoc.h"

#include <I3DEngine.h>
#include <IPhysics.h>
#include ".\terrainvoxelpanel.h"
#include ".\Terrain\Layer.h"
#include "Objects/BrushObject.h"
#include "Objects/VoxelObject.h"

#include "Terrain/TerrainManager.h"

/////////////////////////////////////////////////////////////////////////////
// CTerrainVoxelPanel dialog

const float fMinVoxelSizeStartVal = 0.0625; 

CTerrainVoxelPanel::CTerrainVoxelPanel(CTerrainVoxelTool *tool,CWnd* pParent /*=NULL*/)
	: CDialog(CTerrainVoxelPanel::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTerrainVoxelPanel)
	//}}AFX_DATA_INIT

	m_tool = tool;
	Create( IDD,pParent );

	assert( tool != 0 );

  GetIEditor()->RegisterNotifyListener(this);
}

CTerrainVoxelPanel::~CTerrainVoxelPanel()
{
  GetIEditor()->UnregisterNotifyListener(this);
}

void CTerrainVoxelPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTerrainVoxelPanel)
  DDX_Control(pDX, IDC_VOXEL_SIZE, m_voxelSize);
	DDX_Control(pDX, IDC_BRUSH_TYPE, m_brushType);
	DDX_Control(pDX, IDC_BRUSH_SHAPE, m_brushShape);
	DDX_Control(pDX, IDC_BRUSH_RADIUS_SLIDER, m_radiusSlider);
	DDX_Control(pDX, IDC_SURFACE_TYPES, m_types);
	DDX_Control(pDX, IDC_SETUPPOSITION, m_SetupPosition);
	DDX_Control(pDX, IDC_INITPOSITION, m_InitPosition);
	DDX_Control(pDX, IDC_ALIGN, m_Align);
  DDX_Control(pDX, IDC_COLOR, m_SolidColor);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTerrainVoxelPanel, CDialog)
	//{{AFX_MSG_MAP(CTerrainVoxelPanel)
	ON_WM_HSCROLL()
	ON_EN_UPDATE(IDC_BRUSH_RADIUS, OnUpdateNumbers)
  ON_EN_UPDATE(IDC_BRUSH_DEPTH, OnUpdateNumbers)
  ON_EN_UPDATE(IDC_BRUSH_ELEVATION, OnUpdateNumbers)
  ON_EN_UPDATE(IDC_DETAIL_LEVEL, OnUpdateNumbers)
	ON_EN_UPDATE(IDC_PLANE_SIZE, OnUpdateNumbers)
  ON_CBN_SELENDOK(IDC_VOXEL_SIZE, OnSelendokVoxelSize)
	ON_CBN_SELENDOK(IDC_BRUSH_TYPE, OnSelendokBrushType)
	ON_CBN_SELENDOK(IDC_BRUSH_SHAPE, OnSelendokBrushShape)
	ON_BN_CLICKED(IDC_VOXEL_PHYZ, OnFlagButton)
	ON_BN_CLICKED(IDC_VOXEL_SIMP, OnFlagButton)
	ON_LBN_SELCHANGE(IDC_SURFACE_TYPES, OnSurfTypeSelchange)
	ON_CPN_XT_SELENDOK(IDC_COLOR, OnColorChanged)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_VOXEL_OBJECT, OnBnClickedVoxelObject)
	ON_BN_CLICKED(IDC_VOXEL_TERRAIN, OnBnClickedVoxelTerrain)
	ON_BN_CLICKED(IDC_SETUPPOSITION, OnSetupPosition)
	ON_BN_CLICKED(IDC_INITPOSITION, OnInitPosition)
	ON_BN_CLICKED(IDC_ALIGN, OnAlign)
  ON_BN_CLICKED(IDC_VOXEL_INTEGRATE_SELECTED, OnIntegarteSelected)

  ON_BN_CLICKED(IDC_VOXEL_CREATE, OnBnClickedCreate)
  ON_BN_CLICKED(IDC_VOXEL_SUBTRACT, OnBnClickedSubtract)
  ON_BN_CLICKED(IDC_VOXEL_MATERIAL, OnBnClickedMaterial)
  ON_BN_CLICKED(IDC_VOXEL_INTEGRATE, OnBnClickedIntegrate)
  ON_BN_CLICKED(IDC_VOXEL_BLUR_POS, OnBnClickedBlurPos)
  ON_BN_CLICKED(IDC_VOXEL_BLUR_NEG, OnBnClickedBlurNeg)
  ON_BN_CLICKED(IDC_VOXEL_BLUR_LIMIT_LOD, OnBnClickedLimitLod)
  ON_BN_CLICKED(IDC_VOXEL_COPY_TERRAIN, OnBnClickedCopyTerrain)
  ON_BN_CLICKED(IDC_VOXEL_REGENERATE, OnBnClickedRegenerate)
  ON_BN_CLICKED(IDC_VOXEL_COLOR_TO_LAYER, OnBnClickedBrushSettolayer)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTerrainVoxelPanel message handlers

BOOL CTerrainVoxelPanel::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_brushRadius.Create( this,IDC_BRUSH_RADIUS );
	m_brushDepth.Create( this,IDC_BRUSH_DEPTH );
  m_brushElevation.Create( this,IDC_BRUSH_ELEVATION );
	m_PlaneSize.Create( this,IDC_PLANE_SIZE );

	float fMin = fMinVoxelSizeStartVal;
	float fMax = gEnv->p3DEngine->GetTerrainSize()*2;
	m_brushRadius.SetRange( fMin,fMax);								// number crtl
	m_radiusSlider.SetRange( RadiusToSliderPosition(fMin),RadiusToSliderPosition(fMax) );	// slider
	m_brushDepth.SetRange(0, 1000);
  m_brushElevation.SetRange(0, 8000);
	m_PlaneSize.SetInteger(true);
	m_PlaneSize.SetRange(10, 1000);
	m_PlaneSize.SetValue(m_tool->GetPlaneSize());

  float fSize = fMinVoxelSizeStartVal; 
  for(int i=0; i<12; i++)
  {
    CString str;
    str.Format("%.2f",fSize);
    m_voxelSize.AddString( str );
    fSize *= 2;
  }

  for(int nOperation=1; nOperation<eveoLast; nOperation++)
    m_brushType.AddString(gEnv->p3DEngine->GetVoxelEditOperationName((EVoxelEditOperation)nOperation));

  m_brushType.SetCurSel( 0 );

	m_brushShape.AddString( "Sphere" );
	//m_brushShape.AddString( "Box" );
	m_brushShape.SetCurSel(0);

	CWnd * rb = GetDlgItem(IDC_VOXEL_OBJECT);
	if(rb)
		((CButton *)rb)->SetCheck(true);

	((CButton *) GetDlgItem(IDC_VOXEL_PHYZ))->SetCheck(TRUE);

	SelectVoxelObjectType();	
	OnFlagButton();

	m_SolidColor.SetColor( m_tool->m_vpParams.m_Color );

	ReloadSurfaceTypes();

  OnEditorNotifyEvent( eNotify_OnSelectionChange );

  GetDlgItem(IDC_VOXEL_CREATE)->SetWindowText("Soft Create");
  GetDlgItem(IDC_VOXEL_SUBTRACT)->SetWindowText("Soft Subtract");

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CTerrainVoxelPanel::SelectVoxelObjectType()
{
	if(!m_tool)
		return;

	CButton * rb1 = (CButton *)GetDlgItem(IDC_VOXEL_OBJECT);
	CButton * rb2 = (CButton *)GetDlgItem(IDC_VOXEL_TERRAIN);
	if(rb1)
		rb1->SetCheck(m_tool->m_vpParams.m_voxelType==1);
	if(rb2)
		rb2->SetCheck(m_tool->m_vpParams.m_voxelType==2);

	((CButton *) GetDlgItem(IDC_VOXEL_PHYZ))->SetCheck(m_tool->m_vpParams.m_isUpdatePhysics);
	((CButton *) GetDlgItem(IDC_VOXEL_SIMP))->SetCheck(gEnv->pConsole->GetCVar("r_ShowLines")->GetIVal()==3);

	for(int i=0; i<m_types.GetCount(); i++)
	{
		CString selStr;
		m_types.GetText(i, selStr);
		if(!strcmp(selStr, m_tool->m_vpParams.m_SurfType))
			m_types.SetCurSel(i);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::ReloadSurfaceTypes()
{
	m_types.ResetContent();
	for (int i = 0; i < CTerrainManager::GetTerrainManager().GetSurfaceTypeCount(); i++)
	{
		m_types.InsertString( i, CTerrainManager::GetTerrainManager().GetSurfaceTypePtr(i)->GetName() );
	}

  if(CTerrainManager::GetTerrainManager().GetSurfaceTypeCount())
    m_types.SetCurSel(0);

  for(int i=0; i<m_types.GetCount(); i++)
  {
    CString selStr;
    m_types.GetText(i, selStr);
    if(!strcmp(selStr, m_tool->m_vpParams.m_SurfType))
      m_types.SetCurSel(i);
  }

  OnSurfTypeSelchange();
}


//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::OnColorChanged()
{
	m_tool->m_vpParams.m_Color = m_SolidColor.GetColor();
}


//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::SetBrush( CTerrainVoxelBrush &br, int voxelObjectType )
{
	m_brushType.SetCurSel(br.type-1);

	m_brushRadius.SetValue( br.radius );
	m_radiusSlider.SetPos( RadiusToSliderPosition(br.radius) );
	m_brushDepth.SetValue( br.colision_offset );
  m_brushElevation.SetValue( br.elevation );
	
	if(br.shape==eVoxelBrushShapeSphere)
	{
		m_brushShape.SetCurSel(0);
	}
	if(br.shape==eVoxelBrushShapeBox)
	{
		m_brushShape.SetCurSel(1);
	}

  float fVoxSize = fMinVoxelSizeStartVal; 
  for(int i=0; i<m_voxelSize.GetCount(); i++)
  {
    if(br.minVoxelSize <= fVoxSize || (i==m_voxelSize.GetCount()-1))
    {
      br.minVoxelSize = fVoxSize;
      m_voxelSize.SetCurSel(i);
      break;
    }
    fVoxSize*=2;
  }

	SelectVoxelObjectType();

  ((CButton*)GetDlgItem(IDC_VOXEL_CREATE))->SetCheck(br.type == eveoPaintHeightPos);
  ((CButton*)GetDlgItem(IDC_VOXEL_SUBTRACT))->SetCheck(br.type == eveoPaintHeightNeg);
  ((CButton*)GetDlgItem(IDC_VOXEL_MATERIAL))->SetCheck(br.type == eveoMaterial);
  ((CButton*)GetDlgItem(IDC_VOXEL_INTEGRATE))->SetCheck(br.type == eveoIntegrateMeshPos || br.type == eveoIntegrateMeshNeg);
  ((CButton*)GetDlgItem(IDC_VOXEL_BLUR_POS))->SetCheck(br.type == eveoBlurPos);
  ((CButton*)GetDlgItem(IDC_VOXEL_BLUR_NEG))->SetCheck(br.type == eveoBlurNeg);
  ((CButton*)GetDlgItem(IDC_VOXEL_BLUR_LIMIT_LOD))->SetCheck(br.type == eveoLimitLod);
  ((CButton*)GetDlgItem(IDC_VOXEL_COPY_TERRAIN))->SetCheck(br.type == eveoCopyTerrainPos);

  ((CButton*)GetDlgItem(IDC_VOXEL_INTEGRATE_SELECTED))->EnableWindow(br.type == eveoIntegrateMeshPos || br.type == eveoIntegrateMeshNeg);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::OnUpdateNumbers() 
{
	CTerrainVoxelBrush br;
	m_tool->GetBrush(br);
	float prevRadius = br.radius;
	br.radius = m_brushRadius.GetValue();
	br.colision_offset = m_brushDepth.GetValue();
  br.elevation = m_brushElevation.GetValue();

  br.AutoSetVoxelSize();
	SetBrush( br, -1 );
	m_tool->SetBrush(br);
	m_tool->SetPlaneSize(m_PlaneSize.GetValue());
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::OnFlagButton() 
{
	int f1 = ((CButton *) GetDlgItem(IDC_VOXEL_PHYZ))->GetCheck();
	int f2 = ((CButton *) GetDlgItem(IDC_VOXEL_SIMP))->GetCheck();

	m_tool->m_vpParams.m_isUpdatePhysics = f1;
  gEnv->pConsole->GetCVar("r_ShowLines")->Set(f2 ? 3 : 0);

	gEnv->p3DEngine->Voxel_SetFlags(f1, f2, 0, false);
}

//////////////////////////////////////////////////////////////////////////
float CTerrainVoxelPanel::SliderPositionToRadius(int sliderPosition)
{
	return pow(10.0f, sliderPosition / 100.0f);
}

int CTerrainVoxelPanel::RadiusToSliderPosition(float radius)
{
	return 100 * log10(radius);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::OnHScroll( UINT nSBCode,UINT nPos,CScrollBar* pScrollBar )
{
	CSliderCtrl *pSliderCtrl = (CSliderCtrl*)pScrollBar;
	CTerrainVoxelBrush br;
	m_tool->GetBrush(br);
	if (pSliderCtrl == &m_radiusSlider)
	{
		br.radius = SliderPositionToRadius(m_radiusSlider.GetPos());;
	}

  br.AutoSetVoxelSize();
	SetBrush( br, -1 );
	m_tool->SetBrush(br);
}

void CTerrainVoxelPanel::OnSelendokVoxelSize() 
{
  int sel = m_voxelSize.GetCurSel();
  if (sel == LB_ERR)
    return;

  CTerrainVoxelBrush br;
  m_tool->GetBrush(br);

  float fSize = fMinVoxelSizeStartVal; 
  for(int i=0; i<m_voxelSize.GetCount(); i++)
  {
    if(i == m_voxelSize.GetCurSel())
      br.minVoxelSize = fSize;
    fSize*=2;
  }

  SetBrush( br, -1 );
  m_tool->SetBrush(br);
}

void CTerrainVoxelPanel::OnSelendokBrushType() 
{
	int sel = m_brushType.GetCurSel();
	if (sel != LB_ERR)
		m_tool->SetActiveBrushType(EVoxelEditOperation(sel+1));

  CTerrainVoxelBrush br;
	m_tool->GetBrush(br);
	SetBrush( br, -1 );
}


void CTerrainVoxelPanel::OnSelendokBrushShape() 
{
	int sel = m_brushShape.GetCurSel();
	if (sel != LB_ERR)
	{
		switch (sel)
		{
		case 0:
			m_tool->SetActiveBrushShape(eVoxelBrushShapeSphere);
			break;
		case 1:
			m_tool->SetActiveBrushShape(eVoxelBrushShapeBox);
			break;
		}
	}
	CTerrainVoxelBrush br;
	m_tool->GetBrush(br);
	SetBrush( br, -1 );
}

void CTerrainVoxelPanel::OnBnClickedVoxelObject()
{
	m_tool->SetActiveVoxelObjectType(1);
}

void CTerrainVoxelPanel::OnBnClickedVoxelTerrain()
{
	m_tool->SetActiveVoxelObjectType(2);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::OnSurfTypeSelchange()
{
	int curSel = m_types.GetCurSel();
	if (curSel < 0)
		return;
	m_types.GetText(curSel, m_tool->m_vpParams.m_SurfType );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::OnSetupPosition() 
{
	bool isCheck = m_SetupPosition.GetCheck();
	m_SetupPosition.SetCheck(!isCheck);
	m_tool->SetupPosition(!isCheck);
	m_Align.SetCheck(0);
	m_tool->PlaneAlign(false);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::OnAlign()
{
	bool isCheck = m_Align.GetCheck();
	m_Align.SetCheck(!isCheck);
	m_tool->SetupPosition(false);
	m_SetupPosition.SetCheck(0);
	m_tool->PlaneAlign(!isCheck);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelPanel::OnInitPosition()
{
	m_SetupPosition.SetCheck(false);
	m_tool->InitPosition();
	OnSetupPosition();
}

struct SRNodeIntInfo
{
  SRNodeIntInfo(IRenderNode*_pNode, float _minVoxelSize)
  {
    pNode = _pNode;
    minVoxelSize = _minVoxelSize;
  }
  IRenderNode * pNode;
  float minVoxelSize;
};

void CTerrainVoxelPanel::OnIntegarteSelected() 
{
  if(!gEnv->p3DEngine->GetIVoxTerrain())
	{
		SVoxTerrainInfo info;
		const float terrainSize = (float)gEnv->p3DEngine->GetTerrainSize();
		info.aabbTerrain = AABB(Vec3(0,0,0), Vec3(terrainSize, terrainSize, terrainSize));
		gEnv->p3DEngine->CreateVoxTerrain(info);
	}

  int nSurfaceTypeID=0;

  { // find selected terrain layer
    if(CTerrainManager::GetTerrainManager().GetLayerCount())
      nSurfaceTypeID = CTerrainManager::GetTerrainManager().GetLayer(0)->GetCurrentLayerId();
    else
      nSurfaceTypeID = 0;

    for(int l=0; l<CTerrainManager::GetTerrainManager().GetLayerCount(); l++)
    {
      if(CTerrainManager::GetTerrainManager().GetLayer(l)->IsSelected())
      {
        nSurfaceTypeID = CTerrainManager::GetTerrainManager().GetLayer(l)->GetCurrentLayerId();
        break;
      }
    }
  }

  CTerrainVoxelBrush br;
  m_tool->GetBrush(br);

  int e_VoxTerIntegrateVegetations = 0;
  if(ICVar* pVar = gEnv->pConsole->GetCVar("e_VoxTerIntegrateVegetations"))
    if(pVar->GetIVal())
      e_VoxTerIntegrateVegetations = 1;

  PodArray<SRNodeIntInfo> arrRenderNodesAll;

  int nNumVeg=0, nNumBrush=0;

  // collect marked vegetations
  if(e_VoxTerIntegrateVegetations)
  {
    std::vector<CVegetationInstance*> arrVegetations;
    GetIEditor()->GetVegetationMap()->GetAllInstances(arrVegetations);
    for (int i = 0; i < arrVegetations.size(); i++)
    {
      CVegetationInstance *obj = arrVegetations[i];
      if(obj->pRenderNode && obj->object->IsMarkedForIntegration())
      {
        arrRenderNodesAll.Add(SRNodeIntInfo(obj->pRenderNode,br.minVoxelSize));
        nNumVeg++;
      }
    }
  }

  // collect selected brushes
  for (int i = 0; i < GetIEditor()->GetSelection()->GetCount(); i++)
  {
    CBaseObject *obj = GetIEditor()->GetSelection()->GetObject(i);
    if(obj->GetType() != OBJTYPE_BRUSH)
      continue;

    CBrushObject * pBrush = (CBrushObject *)obj;

    if(IRenderNode * pRNode = obj->GetEngineNode())
    {
      float fSize = fMinVoxelSizeStartVal; 

      for(int nVoxSizeSlot = 0; nVoxSizeSlot<m_voxelSize.GetCount() && nVoxSizeSlot<16; nVoxSizeSlot++)
      {
        float fBrushVoxSize = pBrush->GetIntegrationQuality() ? pBrush->GetIntegrationQuality() : br.minVoxelSize;

        if(fBrushVoxSize <= fSize)
          break;

        fSize*=2;
      }

      arrRenderNodesAll.Add(SRNodeIntInfo(pRNode,fSize));
      nNumBrush++;
    }
  }

  char szMessage[512]="";
  sprintf(szMessage, 
    "This operation will integrate %d brushes and %d vegetations.\n"
    "Are you sure you want to continue?", nNumBrush, nNumVeg);

  if (MessageBox(szMessage, "Meshes integratation", MB_YESNO) == IDNO)
    return;

  // group by integration quality	
  int nFoundNum = 0;
  PodArray<IRenderNode*> arrRenderNodes[16];
  for (int i = 0; i < arrRenderNodesAll.Count(); i++)
  {
    SRNodeIntInfo & rInfo = arrRenderNodesAll[i];

		int nVoxSizeSlot;

		float fSize = fMinVoxelSizeStartVal; 

		for(nVoxSizeSlot = 0; nVoxSizeSlot<m_voxelSize.GetCount() && nVoxSizeSlot<16; nVoxSizeSlot++)
		{	
			if(rInfo.minVoxelSize <= fSize)
				break;

			fSize*=2;
		}

		arrRenderNodes[nVoxSizeSlot].Add(rInfo.pNode);
		nFoundNum++;
  }

  if(!nFoundNum)
  {
    gEnv->pLog->Log("No selected objects found");
    return;
  }

	if(m_tool->m_vpParams.m_isUndoInUse)
	  GetIEditor()->BeginUndo();

  AABB editArea; editArea.Reset();
	if(m_tool->m_vpParams.m_isUndoInUse && CUndo::IsRecording())
		CUndo::Record( new CUndoVoxTerrain( gEnv->p3DEngine->GetIVoxTerrain(), editArea ));

  ColorF cfColor = ColorF((unsigned int) m_tool->m_vpParams.m_Color );

	float fSize = fMinVoxelSizeStartVal; 
	
	for(int nVoxSizeSlot=0; nVoxSizeSlot<16; nVoxSizeSlot++)
	{
		if(arrRenderNodes[nVoxSizeSlot].Count())
			gEnv->p3DEngine->Voxel_Paint(Vec3(0,0,0), 1.f, nSurfaceTypeID, Vec3(cfColor.r,cfColor.g,cfColor.b), 
				br.type, evbsSphere, evetVoxelObjects, &arrRenderNodes[nVoxSizeSlot], fSize);		
		fSize*=2.f;
	}

	if(m_tool->m_vpParams.m_isUndoInUse)
	  GetIEditor()->AcceptUndo( "Integrate objects" );
}

void CTerrainVoxelPanel::OnBnClickedBrushSettolayer()
{
  CLayer *pLayer = 0;

  { // find selected terrain layer
    if(CTerrainManager::GetTerrainManager().GetLayerCount())
      pLayer = CTerrainManager::GetTerrainManager().GetLayer(0);

    for(int l=0; l<CTerrainManager::GetTerrainManager().GetLayerCount(); l++)
    {
      if(CTerrainManager::GetTerrainManager().GetLayer(l)->IsSelected())
      {
        pLayer = CTerrainManager::GetTerrainManager().GetLayer(l);
        break;
      }
    }
  }

  if(!pLayer)
    return;

  pLayer->m_cLayerFilterColor = ColorF((unsigned int)m_SolidColor.GetColor());
}

void CTerrainVoxelPanel::OnBnClickedCreate()
{
  m_brushType.SetCurSel(eveoPaintHeightPos-1);
  OnSelendokBrushType();
}

void CTerrainVoxelPanel::OnBnClickedSubtract()
{
  m_brushType.SetCurSel(eveoPaintHeightNeg-1);
  OnSelendokBrushType();
}

void CTerrainVoxelPanel::OnBnClickedMaterial()
{
  m_brushType.SetCurSel(eveoMaterial-1);
  OnSelendokBrushType();
}

void CTerrainVoxelPanel::OnBnClickedIntegrate()
{
  m_brushType.SetCurSel(eveoIntegrateMeshPos-1);
  OnSelendokBrushType();
}

void CTerrainVoxelPanel::OnBnClickedBlurPos()
{
  m_brushType.SetCurSel(eveoBlurPos-1);
  OnSelendokBrushType();
}

void CTerrainVoxelPanel::OnBnClickedBlurNeg()
{
  m_brushType.SetCurSel(eveoBlurNeg-1);
  OnSelendokBrushType();
}

void CTerrainVoxelPanel::OnBnClickedLimitLod()
{
  m_brushType.SetCurSel(eveoLimitLod-1);
  OnSelendokBrushType();
}

void CTerrainVoxelPanel::OnBnClickedCopyTerrain()
{
  m_brushType.SetCurSel(eveoCopyTerrainPos-1);
  OnSelendokBrushType();
}

void CTerrainVoxelPanel::OnBnClickedRegenerate()
{
  if (MessageBox( "This operation will re-triangulate and optimize entire terrain.\n"
                  "It may take up to few minutes depending on data size.\n"
                  "Are you sure you want to continue?", "Refine terrain", MB_YESNO) == IDNO)
    return;

  gEnv->p3DEngine->Voxel_Paint(Vec3(0,0,0), 1000000, -1, Vec3(0,0,0), 
    eveoLimitLod, evbsSphere, evetVoxelObjects, NULL,0.01f);
}

void CTerrainVoxelPanel::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
  switch (event)
  {
  case eNotify_OnInvalidateControls:
    break;

  case eNotify_OnSelectionChange:
    {
      CCryEditDoc *pDoc = GetIEditor()->GetDocument();

      if(CTerrainManager::GetTerrainManager().GetLayerCount())
        m_SolidColor.SetColor(CTerrainManager::GetTerrainManager().GetLayer(0)->m_cLayerFilterColor.pack_abgr8888()&0xffffff);

      for (int i = 0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
      {
        if(CTerrainManager::GetTerrainManager().GetLayer(i)->IsSelected())
        {
          CLayer * pLayer = CTerrainManager::GetTerrainManager().GetLayer(i);
          m_SolidColor.SetColor(pLayer->m_cLayerFilterColor.pack_abgr8888()&0xffffff);
        }
      }

      OnColorChanged();
    }
    break;
  }
}
