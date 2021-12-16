// TerrainTexture.cpp : implementation file
//

#include "stdafx.h"
#include "TerrainTexture.h"
#include "CryEditDoc.h"
#include "StringDlg.h"
#include "TerrainLighting.h"
#include "DimensionsDialog.h"
#include "NumberDlg.h"
#include ".\Terrain\SurfaceType.h"
#include ".\Terrain\Heightmap.h"
#include ".\Terrain\Layer.h"
#include "VegetationMap.h"

#include ".\Terrain\TerrainTexGen.h"

#include "Material/MaterialManager.h"
#include "Terrain/TerrainManager.h"

#include "GameEngine.h"

// Static member variables
bool CTerrainTextureDialog::m_bUseLighting = true;
bool CTerrainTextureDialog::m_bShowWater = true;

#define IDC_REPORT_CONTROL AFX_IDW_PANE_FIRST+10

#define IDC_TASKPANEL 1
#define IDC_SHOW_PREVIEW_CHECKBOX 10
#define IDC_LAYER_PREVIEW_BUTTON 11

// Task panel commands.
#define CMD_LAYER_ADD             1
#define CMD_LAYER_DELETE          2
#define CMD_LAYER_MOVEUP          3
#define CMD_LAYER_MOVEDOWN        4
#define CMD_LAYER_LOAD_TEXTURE    5
#define CMD_LAYER_ASIGN_MATERIAL  6
#define CMD_OPEN_MATERIAL_EDITOR  7
#define CMD_LAYER_TEXTURE_INFO    8
#define CMD_LAYER_EXPORT_TEXTURE  9

//#define COLUMN_LAYER_ICON  0
#define COLUMN_LAYER_NAME  0
#define COLUMN_MATERIAL    1
#define COLUMN_MIN_HEIGHT  2
#define COLUMN_MAX_HEIGHT  3
#define COLUMN_MIN_ANGLE   4
#define COLUMN_MAX_ANGLE   5
#define COLUMN_BRIGHTNESS  6
#define COLUMN_TILING      7
#define COLUMN_SORT_ORDER  8
#define COLUMN_SPEC_AMOUNT 9
#define COLUMN_USE_REMESH 10


#define TEXTURE_PREVIEW_SIZE 64

//////////////////////////////////////////////////////////////////////////
class CTerrainLayerRecord : public CXTPReportRecord
{
	DECLARE_DYNAMIC(CTerrainLayerRecord)
public:
	CString m_layerName;
	CLayer *m_pLayer;
	CXTPReportRecordItem *m_pVisibleIconItem;

	class CustomPreviewItem : public CXTPReportRecordItemPreview
	{
	public:
		virtual int GetPreviewHeight(CDC* pDC, CXTPReportRow* pRow, int nWidth)
		{
			CXTPImageManagerIcon* pIcon = pRow->GetControl()->GetImageManager()->GetImage(GetIconIndex(), 0);
			if (!pIcon)
				return 0;
			CSize szImage(pIcon->GetWidth(), pIcon->GetHeight());
			return szImage.cy + 6;
		}
	};


	CTerrainLayerRecord( CLayer *pLayer,int nIndex )
	{
		m_pLayer = pLayer;
		m_layerName = pLayer->GetLayerName();

		CString mtlName;
		CSurfaceType *pSurfaceType = pLayer->GetSurfaceType();
		if (pSurfaceType)
		{
			mtlName = pSurfaceType->GetMaterial();
		}

		//CXTPReportRecordItem *pIconItem = AddItem(new CXTPReportRecordItem());
		//pIconItem->SetIconIndex(nIndex);

		AddItem(new CXTPReportRecordItemText(m_layerName))->SetBold(TRUE);
		AddItem(new CXTPReportRecordItemText(mtlName))->AddHyperlink(new CXTPReportHyperlink(0,mtlName.GetLength()));
		AddItem(new CXTPReportRecordItemNumber(m_pLayer->GetLayerStart(),_T("%g")));
		AddItem(new CXTPReportRecordItemNumber(m_pLayer->GetLayerEnd(),_T("%g")));
		AddItem(new CXTPReportRecordItemNumber(m_pLayer->GetLayerMinSlopeAngle(),_T("%g")));
		AddItem(new CXTPReportRecordItemNumber(m_pLayer->GetLayerMaxSlopeAngle(),_T("%g")));
    AddItem(new CXTPReportRecordItemNumber(m_pLayer->GetLayerBrightness(),_T("%g")));
    AddItem(new CXTPReportRecordItemNumber(m_pLayer->GetLayerTiling(),_T("%g")));
    AddItem(new CXTPReportRecordItemNumber(m_pLayer->GetLayerSortOrder(),_T("%g")));
    AddItem(new CXTPReportRecordItemNumber(m_pLayer->GetLayerSpecularAmount(),_T("%g")));
    AddItem(new CXTPReportRecordItemNumber(m_pLayer->GetLayerUseRemeshing(),_T("%g")));

		CustomPreviewItem *pItem = new CustomPreviewItem;
		pItem->SetIconIndex(nIndex);
		SetPreviewItem( pItem );
	}

	virtual void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
	{
		if (pDrawArgs->pColumn)
		{
			DWORD baseColor = ::GetSysColor( COLOR_WINDOW );

			BYTE r = GetRValue( baseColor );
			BYTE g = GetGValue( baseColor );
			BYTE b = GetBValue( baseColor );

			if(pDrawArgs->pColumn->GetIndex() == COLUMN_MATERIAL)
			{
				pItemMetrics->clrBackground = ::GetSysColor(COLOR_GRAYTEXT);
			}
			if (pDrawArgs->pColumn->GetIndex() == COLUMN_MIN_HEIGHT || pDrawArgs->pColumn->GetIndex() == COLUMN_MAX_HEIGHT)
			{
				pItemMetrics->clrBackground = RGB( r * 0.9, g, b * 0.9 );
			}
			else if (pDrawArgs->pColumn->GetIndex() == COLUMN_MIN_ANGLE || pDrawArgs->pColumn->GetIndex() == COLUMN_MAX_ANGLE)
			{
				pItemMetrics->clrBackground = RGB( r * 0.9, g, b );
			}
      else if (pDrawArgs->pColumn->GetIndex() == COLUMN_BRIGHTNESS || pDrawArgs->pColumn->GetIndex() == COLUMN_TILING || pDrawArgs->pColumn->GetIndex() == COLUMN_SORT_ORDER)
      {
				pItemMetrics->clrBackground = RGB( r * 0.9, g * 0.9, b );
      }
      else if (pDrawArgs->pColumn->GetIndex() == COLUMN_SPEC_AMOUNT || pDrawArgs->pColumn->GetIndex() == COLUMN_USE_REMESH)
      {
				pItemMetrics->clrBackground = RGB( r, g * 0.9, b );
      }
		}
	}
};
IMPLEMENT_DYNAMIC(CTerrainLayerRecord,CXTPReportRecord);

/////////////////////////////////////////////////////////////////////////////
// CTerrainTextureDialog dialog


//////////////////////////////////////////////////////////////////////////
class CTerrainLayersUndoObject : public IUndoObject
{
public:
	CTerrainLayersUndoObject()
	{
		m_undo.root = GetISystem()->CreateXmlNode("Undo");
		m_redo.root = GetISystem()->CreateXmlNode("Redo");
		m_undo.bLoading = false;
		CTerrainManager::GetTerrainManager().SerializeLayerSettings( m_undo );
	}
protected:
	virtual int GetSize() { return sizeof(*this); }
	virtual const char* GetDescription() { return "Terrain Layers"; };

	virtual void Undo( bool bUndo )
	{
		if (bUndo)
		{
			m_redo.bLoading = false; // save redo
			CTerrainManager::GetTerrainManager().SerializeLayerSettings( m_redo );
		}
		m_undo.bLoading = true; // load undo
		CTerrainManager::GetTerrainManager().SerializeLayerSettings( m_undo );

		CTerrainTextureDialog::OnUndoUpdate();
		GetIEditor()->Notify(eNotify_OnInvalidateControls);
	}
	virtual void Redo()
	{
		m_redo.bLoading = true; // load redo
		CTerrainManager::GetTerrainManager().SerializeLayerSettings( m_redo );

		CTerrainTextureDialog::OnUndoUpdate();
		GetIEditor()->Notify(eNotify_OnInvalidateControls);
	}

private:
	CXmlArchive m_undo;
	CXmlArchive m_redo;
};
//////////////////////////////////////////////////////////////////////////



IMPLEMENT_DYNCREATE(CTerrainTextureDialog,CToolbarDialog)

//////////////////////////////////////////////////////////////////////////
class CTerrainTextureDialogClassDesc : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {A5665E6A-A02B-485f-BC12-7B157DBA841A}
		static const GUID guid = { 0xa5665e6a, 0xa02b, 0x485f, { 0xbc, 0x12, 0x7b, 0x15, 0x7d, 0xba, 0x84, 0x1a } };
		return guid;
	}
	virtual const char* ClassName() { return "Terrain Texture Layers"; };
	virtual const char* Category() { return "Terrain Texture Layers"; };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTerrainTextureDialog); };
	virtual const char* GetPaneTitle() { return _T("Terrain Texture Layers"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(100,100,1000,800); };
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return false; };
};

REGISTER_CLASS_DESC( CTerrainTextureDialogClassDesc );

static CTerrainTextureDialog* m_gTerrainTextureDialog = 0;

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnUndoUpdate()
{
	if (m_gTerrainTextureDialog)
		m_gTerrainTextureDialog->ReloadLayerList();
}

//////////////////////////////////////////////////////////////////////////
CTerrainTextureDialog::CTerrainTextureDialog(CWnd* pParent /*=NULL*/)
	: CToolbarDialog(CTerrainTextureDialog::IDD, pParent), m_ar("LayerSettings")
	, m_pAssignMaterialLink( NULL )
{
	static bool bFirstInstance = true;

	m_gTerrainTextureDialog = this;

	// No current layer at first
	m_pCurrentLayer = NULL;

	// Paint it gray
	if (bFirstInstance)
	{
		bFirstInstance = false; // Leave it the next time
	}

	Create( CTerrainTextureDialog::IDD,AfxGetMainWnd() );

	m_bIgnoreNoitify = false;
	GetIEditor()->RegisterNotifyListener( this );
	GetIEditor()->GetMaterialManager()->AddListener( this );
}

//////////////////////////////////////////////////////////////////////////
CTerrainTextureDialog::~CTerrainTextureDialog()
{
	GetIEditor()->GetMaterialManager()->RemoveListener( this );
	GetIEditor()->UnregisterNotifyListener( this );
	ClearData();

	m_gTerrainTextureDialog = 0;

	CompressLayers();

	//CTerrainManager::GetTerrainManager().GetHeightmap().UpdateEngineTerrain(0,0,
	//	CTerrainManager::GetTerrainManager().GetHeightmap().GetWidth(),
	//	CTerrainManager::GetTerrainManager().GetHeightmap().GetHeight(),false,true);

	GetIEditor()->UpdateViews(eUpdateHeightmap);
}

void CTerrainTextureDialog::ClearData()
{
	if (m_wndReport.m_hWnd)
	{
		m_wndReport.GetSelectedRows()->Clear();
		m_wndReport.GetRecords()->RemoveAll();
	}

	// Release all layer masks.
	for (int i=0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
		CTerrainManager::GetTerrainManager().GetLayer(i)->ReleaseTempResources();

	m_pCurrentLayer = 0;
}


BEGIN_MESSAGE_MAP(CTerrainTextureDialog, CToolbarDialog)
	//{{AFX_MSG_MAP(CTerrainTextureDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_LOAD_TEXTURE, OnLoadTexture)
	ON_BN_CLICKED(IDC_IMPORT, OnImport)
	ON_BN_CLICKED(IDC_EXPORT, OnExport)
	ON_BN_CLICKED(ID_FILE_REFINETERRAINTEXTURETILES, OnRefineTerrainTextureTiles)
	ON_BN_CLICKED(ID_FILE_EXPORTLARGEPREVIEW, OnFileExportLargePreview)
	ON_COMMAND(ID_PREVIEW_APPLYLIGHTING, OnApplyLighting)
	ON_COMMAND(ID_LAYER_SETWATERLEVEL, OnSetWaterLevel)
	ON_COMMAND(ID_LAYER_EXPORTTEXTURE, OnLayerExportTexture)

	ON_BN_CLICKED(IDC_TTS_HOLD, OnHold)
	ON_BN_CLICKED(IDC_TTS_FETCH, OnFetch)
	ON_BN_CLICKED(IDC_USE_LAYER, OnUseLayer)
	ON_COMMAND(ID_OPTIONS_SETLAYERBLENDING, OnOptionsSetLayerBlending)
	ON_BN_CLICKED(IDC_AUTO_GEN_MASK, OnAutoGenMask)
	ON_BN_CLICKED(IDC_LOAD_MASK, OnLoadMask)
	ON_BN_CLICKED(IDC_EXPORT_MASK, OnExportMask)
	ON_COMMAND(ID_PREVIEW_SHOWWATER, OnShowWater)

	ON_BN_CLICKED(IDC_SHOW_PREVIEW_CHECKBOX, OnShowPreviewCheckbox)
	
	ON_MESSAGE(XTPWM_TASKPANEL_NOTIFY, OnTaskPanelNotify)

	ON_NOTIFY(NM_CLICK, IDC_REPORT_CONTROL, OnReportClick)
	ON_NOTIFY(NM_RCLICK, IDC_REPORT_CONTROL, OnReportRClick)
	ON_NOTIFY(XTP_NM_REPORT_SELCHANGED, IDC_REPORT_CONTROL, OnReportSelChange)
	ON_NOTIFY(XTP_NM_REPORT_HYPERLINK, IDC_REPORT_CONTROL, OnReportHyperlink)
	ON_NOTIFY(XTP_NM_REPORT_VALUECHANGED, IDC_REPORT_CONTROL, OnReportPropertyChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTerrainTextureDialog message handlers

BOOL CTerrainTextureDialog::OnInitDialog() 
{
	VERIFY( InitCommandBars() );
	CXTPCommandBar* pMenuBar = GetCommandBars()->SetMenu( _T("Menu Bar"),IDR_LAYER );	
	pMenuBar->SetFlags(xtpFlagStretched);
	pMenuBar->EnableCustomization(FALSE);

	m_pCurrentLayer = 0;
	m_doc = GetIEditor()->GetDocument();

	CSliderCtrl ctrlSlider;

	__super::OnInitDialog();

	CWaitCursor wait;
	
	// Update the menus
	GetMenu()->GetSubMenu(2)->CheckMenuItem(ID_PREVIEW_APPLYLIGHTING,m_bUseLighting ? MF_CHECKED : MF_UNCHECKED);
	GetMenu()->GetSubMenu(2)->CheckMenuItem(ID_PREVIEW_SHOWWATER, m_bShowWater ? MF_CHECKED : MF_UNCHECKED);
	
	// Invalidate layer masks.
	for (int i=0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
	{
		// Add the name of the layer
		CTerrainManager::GetTerrainManager().GetLayer(i)->InvalidateMask();
	}

	InitTaskPanel();
	InitReportCtrl();

	// Load the layer list from the document
	m_wndReport.GetPaintManager()->m_clrHyper = ::GetSysColor(COLOR_HIGHLIGHT);
	ReloadLayerList();

	OnGeneratePreview();

	CTerrainManager::GetTerrainManager().SerializeLayerSettings(m_ar);

	RecalcLayout();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::InitReportCtrl()
{
	CRect rc;
	ScreenToClient(rc);

	class CTTDReportPaintManager : public CXTPReportPaintManager
	{
	protected:
		int DrawLink(int* pnCurrDrawPos, XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, CXTPReportHyperlink* pHyperlink, CString strText, CRect rcLink, int nFlag)
		{
			CFont * pFont = 0;
			CFont * pOldFont = 0;
			if(pDrawArgs && pDrawArgs->pDC && (pFont = pDrawArgs->pDC->GetCurrentFont()))
			{
				if(!HFONT(newFont))
				{
					LOGFONT logFont;
					pFont->GetLogFont(&logFont);
					logFont.lfUnderline = TRUE;
					newFont.CreateFontIndirect(&logFont);
				}
				pOldFont = pDrawArgs->pDC->SelectObject(&newFont);
			}

			int nRes = CXTPReportPaintManager::DrawLink(pnCurrDrawPos, pDrawArgs, pHyperlink, strText, rcLink, nFlag);
			if(pFont)
				pDrawArgs->pDC->SelectObject(pOldFont);
			return nRes;
		}
	private:
		CFont newFont;
	};
	m_wndReport.SetPaintManager(new CTTDReportPaintManager());

	VERIFY( m_wndReport.Create(WS_CHILD|WS_TABSTOP|WS_VISIBLE|WM_VSCROLL, rc, this, IDC_REPORT_CONTROL) );
	m_wndReport.ModifyStyleEx( 0,WS_EX_STATICEDGE );

	//  Add sample columns
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_LAYER_NAME, _T("Layer"), 150, TRUE,XTP_REPORT_NOICON,FALSE ))->SetEditable(TRUE);
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_MATERIAL, _T("Material"), 300, TRUE,XTP_REPORT_NOICON,FALSE))->SetEditable(FALSE);
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_MIN_HEIGHT, _T("Min Height"), 70, FALSE,XTP_REPORT_NOICON,FALSE))->SetAlignment(DT_CENTER);
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_MAX_HEIGHT, _T("Max Height"), 70, FALSE,XTP_REPORT_NOICON,FALSE))->SetAlignment(DT_CENTER);
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_MIN_ANGLE, _T("Min Angle"), 70, FALSE,XTP_REPORT_NOICON,FALSE))->SetAlignment(DT_CENTER);
	m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_MAX_ANGLE, _T("Max Angle"), 70, FALSE,XTP_REPORT_NOICON,FALSE))->SetAlignment(DT_CENTER);
  m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_BRIGHTNESS, _T("Brightness"), 70, FALSE,XTP_REPORT_NOICON,FALSE))->SetAlignment(DT_CENTER);
  m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_TILING, _T("Base Tiling (test)"), 70, FALSE,XTP_REPORT_NOICON,FALSE))->SetAlignment(DT_CENTER);
  m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_SORT_ORDER, _T("Sort order (test)"), 70, FALSE,XTP_REPORT_NOICON,FALSE))->SetAlignment(DT_CENTER);
  m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_SPEC_AMOUNT, _T("Specular Amount (test)"), 70, FALSE,XTP_REPORT_NOICON,FALSE))->SetAlignment(DT_CENTER);
  m_wndReport.AddColumn(new CXTPReportColumn(COLUMN_USE_REMESH, _T("UseRemeshing"), 70, FALSE,XTP_REPORT_NOICON,FALSE))->SetAlignment(DT_CENTER);

	m_wndReport.SetMultipleSelection(TRUE);
	m_wndReport.SkipGroupsFocus(TRUE);
	m_wndReport.AllowEdit(TRUE);
	m_wndReport.EditOnClick(FALSE);

	m_wndReport.EnablePreviewMode(TRUE);
	//////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::InitTaskPanel()
{
	CRect rc(0,0,0,0);
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// Task panel.
	//////////////////////////////////////////////////////////////////////////
	m_wndTaskPanel.Create( WS_CHILD|WS_BORDER|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,rc,this,IDC_TASKPANEL );

	//m_taskImageList.Create( IDR_DB_GAMETOKENS_BAR,16,1,RGB(192,192,192) );
	//VERIFY( CMFCUtils::LoadTrueColorImageList( m_taskImageList,IDR_DB_GAMETOKENS_BAR,15,RGB(192,192,192) ) );
	//m_wndTaskPanel.SetImageList( &m_taskImageList );
	m_wndTaskPanel.SetBehaviour(xtpTaskPanelBehaviourExplorer);
	m_wndTaskPanel.SetTheme(xtpTaskPanelThemeNativeWinXP);
	m_wndTaskPanel.SetSelectItemOnFocus(TRUE);
	m_wndTaskPanel.AllowDrag(TRUE);
	m_wndTaskPanel.SetAnimation(xtpTaskPanelAnimationNo);


	//////////////////////////////////////////////////////////////////////////
	m_showPreviewCheck.Create( "Show Preview",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,IDC_SHOW_PREVIEW_CHECKBOX );
	m_showPreviewCheck.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_showPreviewCheck.SetParent(&m_wndTaskPanel); m_showPreviewCheck.SetOwner(this); // Akward but needed to route msgs to this dialog
	m_showPreviewCheck.SetCheck(TRUE);

	m_previewLayerTextureButton.Create( "",WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_CENTERIMAGE,CRect(0,0,130,130),&m_wndTaskPanel,IDC_LAYER_PREVIEW_BUTTON );
	//////////////////////////////////////////////////////////////////////////

	// Add default tasks.
	CXTPTaskPanelGroupItem *pItem =  NULL;
	CXTPTaskPanelGroup *pGroup = NULL;
	{
		pGroup = m_wndTaskPanel.AddGroup(1);
		pGroup->SetCaption( "Layer Tasks" );

		pItem =  pGroup->AddLinkItem(CMD_LAYER_ADD); pItem->SetType(xtpTaskItemTypeLink); pItem->SetCaption( "Add Layer" );
		pItem =  pGroup->AddLinkItem(CMD_LAYER_DELETE);  pItem->SetCaption( "Delete Layer" );
		pItem =  pGroup->AddLinkItem(CMD_LAYER_MOVEUP); pItem->SetType(xtpTaskItemTypeLink); pItem->SetCaption( "Move Layer Up" );
		pItem =  pGroup->AddLinkItem(CMD_LAYER_MOVEDOWN); pItem->SetType(xtpTaskItemTypeLink); pItem->SetCaption( "Move Layer Down" );
		m_pAssignMaterialLink =  pGroup->AddLinkItem(CMD_LAYER_ASIGN_MATERIAL); m_pAssignMaterialLink->SetType(xtpTaskItemTypeLink); m_pAssignMaterialLink->SetCaption( "Assign Material" );
		UpdateAssignMaterialItem();
	}

	{
		pGroup = m_wndTaskPanel.AddGroup(2);
		pGroup->SetCaption( "Layer Info" );
		m_pLayerInfoText = pGroup->AddTextItem("Layer Info");
	}

	{
		pGroup = m_wndTaskPanel.AddGroup(2);
		pGroup->SetCaption( "Layer Texture" );

		pItem =  pGroup->AddLinkItem(CMD_LAYER_LOAD_TEXTURE); pItem->SetType(xtpTaskItemTypeLink); pItem->SetCaption( "Change Layer Texture" );
		pItem =  pGroup->AddControlItem(m_previewLayerTextureButton);
		m_pTextureInfoText = pGroup->AddTextItem("Texture Info");
	}

	{
		pGroup = m_wndTaskPanel.AddGroup(3);
		pGroup->SetCaption( "Options" );
		pItem =  pGroup->AddControlItem(m_showPreviewCheck);
	}
	//////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::ReloadLayerList()
{
	////////////////////////////////////////////////////////////////////////
	// Fill the layer list box with the data from the document
	////////////////////////////////////////////////////////////////////////

	unsigned int i;

	//////////////////////////////////////////////////////////////////////////
	// Populate report control
	//////////////////////////////////////////////////////////////////////////
	m_wndReport.SetRedraw(FALSE);

	GeneratePreviewImageList();

	m_wndReport.GetSelectedRows()->Clear();
	m_wndReport.GetRecords()->RemoveAll();
	m_wndReport.SetRedraw(TRUE);


	for (i=0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
	{
		CLayer *pLayer = CTerrainManager::GetTerrainManager().GetLayer(i);
		m_wndReport.AddRecord( new CTerrainLayerRecord(pLayer,i) );
	}

	m_wndReport.EnablePreviewMode( m_showPreviewCheck.GetCheck() == BST_CHECKED );

	m_wndReport.Populate();

	m_wndReport.GetSelectedRows()->Clear();
	int count = m_wndReport.GetRows()->GetCount();
	for (int i = 0; i < count; i++)
	{
		CTerrainLayerRecord *pRec = DYNAMIC_DOWNCAST(CTerrainLayerRecord,m_wndReport.GetRows()->GetAt(i)->GetRecord());
		if (pRec && pRec->m_pLayer && pRec->m_pLayer->IsSelected())
		{
			if (!m_pCurrentLayer)
				m_pCurrentLayer = pRec->m_pLayer;
			m_wndReport.GetSelectedRows()->Add( m_wndReport.GetRows()->GetAt(i) );
		}
	}

	m_wndReport.SetRedraw(TRUE);
	//////////////////////////////////////////////////////////////////////////

	UpdateControlData();

  CTerrainManager::GetTerrainManager().ReloadSurfaceTypes();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnSelchangeLayerList()
{
	CLayer *pLayer = NULL;
	// Get the layer associated with the selection
	POSITION pos = m_wndReport.GetSelectedRows()->GetFirstSelectedRowPosition();
	while (pos)
	{
		CTerrainLayerRecord *pRec = DYNAMIC_DOWNCAST(CTerrainLayerRecord,m_wndReport.GetSelectedRows()->GetNextSelectedRow( pos )->GetRecord());
		if (!pRec)
			continue;
		pLayer = CTerrainManager::GetTerrainManager().FindLayer( pRec->m_layerName );
		if (pLayer)
			break;
	}

	// Unselect all layers.
	for (int i = 0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
	{
		CTerrainManager::GetTerrainManager().GetLayer(i)->SetSelected(false);
	}

	// Set it as the current one
	m_pCurrentLayer = pLayer;

	if (m_pCurrentLayer)
		m_pCurrentLayer->SetSelected(true);

//	m_bMaskPreviewValid = false;

	// Update the controls with the data from the layer
	UpdateControlData();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::CompressLayers()
{
	for (int i=0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
	{
		CLayer *layer = CTerrainManager::GetTerrainManager().GetLayer(i);

		layer->CompressMask();
	}
}


void CTerrainTextureDialog::EnableControls()
{
	////////////////////////////////////////////////////////////////////////
	// Enable / disable the current based of if at least one layer is
	// present and activated
	////////////////////////////////////////////////////////////////////////

	BOOL bEnable = m_pCurrentLayer != NULL;
	if (CMenu *pMenu=GetMenu())
	{
		if (CMenu *pSubmenu=pMenu->GetSubMenu(1))
		{
			pSubmenu->EnableMenuItem(IDC_LOAD_TEXTURE,bEnable ? MF_ENABLED : MF_GRAYED);
			pSubmenu->EnableMenuItem(ID_LAYER_EXPORTTEXTURE, bEnable ? MF_ENABLED : MF_GRAYED);
			pSubmenu->EnableMenuItem(IDC_REMOVE_LAYER, bEnable ? MF_ENABLED : MF_GRAYED);
		}
	}

	BOOL bHaveLayers = CTerrainManager::GetTerrainManager().GetLayerCount() != 0 ? TRUE : FALSE;
	
	
	/*
	// Only enable export and generate preview option when we have layer(s)
	GetDlgItem(IDC_EXPORT)->EnableWindow( bHaveLayers );
	GetMenu()->GetSubMenu(0)->EnableMenuItem(IDC_EXPORT, bHaveLayers ? MF_ENABLED : MF_GRAYED);
	GetMenu()->GetSubMenu(2)->EnableMenuItem(IDC_GENERATE_PREVIEW,bHaveLayers ? MF_ENABLED : MF_GRAYED);
	GetMenu()->GetSubMenu(2)->EnableMenuItem(ID_FILE_EXPORTLARGEPREVIEW, bHaveLayers ? MF_ENABLED : MF_GRAYED);
	*/
}

void CTerrainTextureDialog::UpdateControlData()
{
	////////////////////////////////////////////////////////////////////////	
	// Update the controls with the data from the current layer
	////////////////////////////////////////////////////////////////////////	

	CSliderCtrl ctrlSlider;
	CButton ctrlButton;

	CString maskInfoText;

	Layers layers;
	GetSelectedLayers(layers);

	if (layers.size() == 1)
	{
		CLayer *pSelLayer = layers[0];
		// Allocate the memory for the texture

		CImage preview;
		preview.Allocate( 128,128 );
		if (pSelLayer->GetTexturePreviewImage().IsValid())
			CImageUtil::ScaleToFit( pSelLayer->GetTexturePreviewImage(),preview );
		preview.SwapRedAndBlue();
		CBitmap bmp;
		VERIFY(bmp.CreateBitmap(128,	128, 1, 32, NULL));
		bmp.SetBitmapBits( preview.GetSize(),(DWORD*)preview.GetData() );
		m_previewLayerTextureButton.SetBitmap(bmp);

		CString textureInfoText;

		textureInfoText.Format( "%s\n%i x %i LayerId %i", LPCTSTR(pSelLayer->GetTextureFilename()),
      pSelLayer->GetTextureWidth(),pSelLayer->GetTextureHeight(), pSelLayer->GetCurrentLayerId());
		m_pTextureInfoText->SetCaption( textureInfoText );

		int nNumSurfaceTypes = CTerrainManager::GetTerrainManager().GetSurfaceTypeCount();

		CString maskInfoText;
		int maskRes = pSelLayer->GetMaskResolution();
		maskInfoText.Format( "Layer Mask: %dx%d\nSurface Type Count %d",maskRes,maskRes,nNumSurfaceTypes );

		//if(m_doc->IsNewTerranTextureSystem())
		//maskInfoText.Format("RGB(%dx%d)",m_doc->GetHeightmap().m_TerrainRGBTexture.CalcMinRequiredTextureExtend(),m_doc->GetHeightmap().m_TerrainRGBTexture.CalcMinRequiredTextureExtend() );
		m_pLayerInfoText->SetCaption( maskInfoText );

	}
	else
	{
	}

	// Update the controls
	EnableControls();
}

void CTerrainTextureDialog::OnLoadTexture() 
{
	if (!m_pCurrentLayer)
		return;
	////////////////////////////////////////////////////////////////////////
	// Load a texture from a BMP file
	////////////////////////////////////////////////////////////////////////
	CString file = m_pCurrentLayer->GetTextureFilenameWithPath();

	//bool res = CFileUtil::SmartSelectSingleFile( file,"",Path::MakeGamePath(Path::GetPath(file)) );
	bool res = CFileUtil::SelectSingleFile( EFILE_TYPE_TEXTURE,file );

	if (res) 
	{
		CUndo undo("Load Layer Texture");
		GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

		// Load the texture
		if (!m_pCurrentLayer->LoadTexture( file ))
			AfxMessageBox("Error while loading the texture !");

		ReloadLayerList();
	}

	// Regenerate the preview
	OnGeneratePreview();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnGeneratePreview()
{
	////////////////////////////////////////////////////////////////////////
	// Generate all layer mask and create a preview of the texture
	////////////////////////////////////////////////////////////////////////

	// Allocate the memory for the texture
	CImage preview;
	preview.Allocate( FINAL_TEX_PREVIEW_PRECISION_CX,FINAL_TEX_PREVIEW_PRECISION_CY );

	// Generate the surface texture
	int tflags = ETTG_ABGR|ETTG_USE_LIGHTMAPS;
	if (m_bUseLighting)
		tflags |= ETTG_LIGHTING;
	if (m_bShowWater)
		tflags |= ETTG_SHOW_WATER;

	// Write the texture data into the bitmap
	//m_bmpFinalTexPrev.SetBitmapBits( preview.GetSize(),(DWORD*)preview.GetData() );
}

void CTerrainTextureDialog::OnFileExportLargePreview()
{
	////////////////////////////////////////////////////////////////////////
	// Show a large preview of the final texture
	////////////////////////////////////////////////////////////////////////
	
	bool bReturn;
	CDimensionsDialog cDialog;
	
	// 1024x1024 is default
	cDialog.SetDimensions(1024);

	// Query the size of the preview
	if (cDialog.DoModal() == IDCANCEL)
		return;

	CLogFile::FormatLine("Exporting large surface texture preview (%ix%i)...", 
		cDialog.GetDimensions(), cDialog.GetDimensions());

	// Allocate the memory for the texture
	CImage image;
	if (!image.Allocate( cDialog.GetDimensions(),cDialog.GetDimensions() ))
		return;

	// Generate the surface texture
	int tflags = ETTG_INVALIDATE_LAYERS|ETTG_STATOBJ_SHADOWS|ETTG_BAKELIGHTING;
	if (m_bUseLighting)
		tflags |= ETTG_LIGHTING;
	if (m_bShowWater)
		tflags |= ETTG_SHOW_WATER;

	CTerrainTexGen texGen;
	bReturn = texGen.GenerateSurfaceTexture( tflags,image );

	{
		unsigned int *pPixData = image.GetData(), *pPixDataEnd = &(image.GetData())[image.GetWidth()*image.GetHeight()];

		// Switch R and B
		while(pPixData != pPixDataEnd)
		{
			// Extract the bits, shift them, put them back and advance to the next pixel
			*pPixData++ = ((* pPixData & 0x00FF0000) >> 16) | (* pPixData & 0x0000FF00) | ((* pPixData & 0x000000FF) << 16);
		}
	}
 
	if (!bReturn)
	{
		CLogFile::WriteLine("Error while generating surface texture preview");
		AfxMessageBox("Can't generate preview !");
	}
	else
	{
		GetIEditor()->SetStatusText("Saving preview...");
		
		// Save the texture to disk

		CString				strImagePath(gSettings.strStandardTempDirectory);
		CString				strImageFilename("TexturePreview.bmp");

		Path::AddBackslash(strImagePath);
		strImagePath+=strImageFilename;

		CFileUtil::CreateDirectory( gSettings.strStandardTempDirectory.GetBuffer() );
		bReturn = CImageUtil::SaveImage( strImagePath, image );

		if (!bReturn)
		{
			AfxMessageBox("Can't save preview bitmap !");
		}
	}

	if (bReturn)
	{
		CString				strImageFilename("TexturePreview.bmp");
		CString				dir(gSettings.strStandardTempDirectory);
		Path::AddBackslash(dir);
		// Show the texture
		::ShellExecute(::GetActiveWindow(), "open", strImageFilename.GetBuffer(),"",dir, SW_SHOWMAXIMIZED);
	}

	// Reset the status text
	GetIEditor()->SetStatusText("Ready");
}


/*
bool CTerrainTextureDialog::GenerateSurface(DWORD *pSurface, UINT iWidth, UINT iHeight, int flags,CBitArray *pLightingBits, float **ppHeightmapData)
{
	////////////////////////////////////////////////////////////////////////
	// Generate the surface texture with the current layer and lighting
	// configuration and write the result to pSurface. Also give out the
	// results of the terrain lighting if pLightingBit is not NULL. Also,
	// if ppHeightmapData is not NULL, the allocated heightmap data will
	// be stored there instead destroing it at the end of the function
	////////////////////////////////////////////////////////////////////////
	bool bUseLighting = flags & GEN_USE_LIGHTING;
	bool bShowWater = flags & GEN_SHOW_WATER;
	bool bShowWaterColor = flags & GEN_SHOW_WATERCOLOR;
  bool bConvertToABGR = flags & GEN_ABGR;
	bool bCalcStatObjShadows = flags & GEN_STATOBJ_SHADOWS;
	bool bKeepLayerMasks = flags & GEN_KEEP_LAYERMASKS;

	CHeightmap *heightmap = GetIEditor()->GetHeightmap();
	float waterLevel = heightmap->GetWaterLevel();

	uint32 i, iTexX, iTexY;
	char szStatusBuffer[128];
	bool bGenPreviewTexture = true;
	COLORREF crlfNewCol;
	CBrush brshBlack(BLACK_BRUSH);
 	int iBlend;
	CTerrainLighting cLighting;
	DWORD *pTextureDataWrite = NULL;
	CLayer *pLayerShortcut = NULL;
	int iFirstUsedLayer;
	float *pHeightmapData = NULL;

	ASSERT(iWidth);
	ASSERT(iHeight);
	ASSERT(!IsBadWritePtr(pSurface, iWidth * iHeight * sizeof(DWORD)));

	if (iWidth == 0 || iHeight == 0)
		return false;

	m_doc = GetIEditor()->GetDocument();


	// Display an hourglass cursor
	BeginWaitCursor();

	CLogFile::WriteLine("Generating texture surface...");

	////////////////////////////////////////////////////////////////////////
	// Search for the first layer that is used
	////////////////////////////////////////////////////////////////////////

	iFirstUsedLayer = -1;

	for (i=0; i < m_doc->GetLayerCount(); i++)
	{
		// Have we founf the first used layer ?
		if (m_doc->GetLayer(i)->IsInUse())
		{
			iFirstUsedLayer = i;
			break;
		}
	}

	// Abort if there is no used layer
	if (iFirstUsedLayer == -1)
		return false;

	////////////////////////////////////////////////////////////////////////
	// Generate the layer masks
	////////////////////////////////////////////////////////////////////////
	
	// Status message
	GetIEditor()->SetStatusText("Scaling heightmap...");
		
	// Allocate memory for the heightmap data
	pHeightmapData = new float[iWidth * iHeight];
	assert(pHeightmapData);
	
	// Retrieve the heightmap data
	m_doc->m_cHeightmap.GetDataEx(pHeightmapData, iWidth, true, true);

	int t0 = GetTickCount();

	bool bProgress = iWidth >= 1024;

	CWaitProgress wait( "Blending Layers",bProgress );

	CLayer *tempWaterLayer = 0;
	if (bShowWater)
	{
		// Apply water level.
		// Add a temporary water layer to the list
		tempWaterLayer = new CLayer;
		//water->LoadTexture(MAKEINTRESOURCE(IDB_WATER), 128, 128);
		tempWaterLayer->FillWithColor( m_doc->GetWaterColor(),8,8 );
		tempWaterLayer->GenerateWaterLayer16(pHeightmapData,iWidth, iHeight, waterLevel );
		m_doc->AddLayer( tempWaterLayer );
	}

	CByteImage layerMask;

	////////////////////////////////////////////////////////////////////////
	// Generate the masks and the texture.
	////////////////////////////////////////////////////////////////////////
	int numLayers = m_doc->GetLayerCount();
	for (i=iFirstUsedLayer; i<(int) numLayers; i++)
	{
		CLayer *layer = m_doc->GetLayer(i);

		// Skip the layer if it is not in use
		if (!layer->IsInUse())
			continue;

		if (!layer->HasTexture())
			continue;

		if (bProgress)
		{
			wait.Step( i*100/numLayers );
		}

		// Status message
		sprintf(szStatusBuffer, "Updating layer %i of %i...", i + 1, m_doc->GetLayerCount());
		GetIEditor()->SetStatusText(szStatusBuffer);

		// Cache surface texture in.
		layer->PrecacheTexture();
		
		// Generate the mask for the current layer
		if (i != iFirstUsedLayer)
		{
			CFloatImage hmap;
			hmap.Attach( pHeightmapData,iWidth,iHeight );
			// Generate a normal layer from the user's parameters, stream from disk if it exceeds a given size
			if (!layer->UpdateMask( hmap,layerMask ))
				continue;
		}
		sprintf(szStatusBuffer, "Blending layer %i of %i...", i + 1, m_doc->GetLayerCount());
		GetIEditor()->SetStatusText(szStatusBuffer);

		// Set the write pointer (will be incremented) for the surface data
		DWORD *pTex = pSurface;

		uint32 layerWidth = layer->GetTextureWidth();
		uint32 layerHeight = layer->GetTextureHeight();

		if (i == iFirstUsedLayer)
		{
			// Draw the first layer, without layer mask.
			for (iTexY=0; iTexY < iHeight; iTexY++)
			{
				uint32 layerY = iTexY % layerHeight;
				for (iTexX=0; iTexX < iWidth; iTexX++)
				{
					// Get the color of the tiling texture at this position
					*pTex++ = layer->GetTexturePixel( iTexX % layerWidth,layerY );
				}
			}
		}
		else
		{
			// Draw the current layer with layer mask.
			for (iTexY=0; iTexY < iHeight; iTexY++)
			{
				uint32 layerY = iTexY % layerHeight;
				for (iTexX=0; iTexX < iWidth; iTexX++)
				{
					// Scale the current preview coordinate to the layer mask and get the value.
					iBlend = layerMask.ValueAt(iTexX,iTexY);
					// Check if this pixel should be drawn.
					if (iBlend == 0)
					{
						pTex++;
						continue;
					}
					
					// Get the color of the tiling texture at this position
					crlfNewCol = layer->GetTexturePixel( iTexX % layerWidth,layerY );
					
					// Just overdraw when the opaqueness of the new layer is maximum
					if (iBlend == 255)
					{
						*pTex = crlfNewCol;
					}
					else
					{
						// Blend the layer into the existing color, iBlend is the blend factor taken from the layer
						*pTex =  (((255 - iBlend) * (*pTex & 0x000000FF)	+  (crlfNewCol & 0x000000FF)        * iBlend) >> 8)      |
							((((255 - iBlend) * (*pTex & 0x0000FF00) >>  8) + ((crlfNewCol & 0x0000FF00) >>  8) * iBlend) >> 8) << 8 |
							((((255 - iBlend) * (*pTex & 0x00FF0000) >> 16) + ((crlfNewCol & 0x00FF0000) >> 16) * iBlend) >> 8) << 16;
					}
					pTex++;
				}
			}
		}

		if (!bKeepLayerMasks)
		{
			layer->ReleaseTempResources();
		}
	}

	if (tempWaterLayer)
	{
		m_doc->RemoveLayer(tempWaterLayer);
	}

	int t1 = GetTickCount();
	CLogFile::FormatLine( "Texture surface layers blended in %dms",t1-t0 );

	if (bProgress)
		wait.Stop();

//	CString str;
//	str.Format( "Time %dms",t1-t0 );
//	MessageBox( str,"Time",MB_OK );

	////////////////////////////////////////////////////////////////////////
	// Light the texture
	////////////////////////////////////////////////////////////////////////
	
	if (bUseLighting)
	{
		CByteImage *shadowMap = 0;
		if (bCalcStatObjShadows)
		{
			CLogFile::WriteLine("Generating shadows of static objects..." );
			GetIEditor()->SetStatusText( "Generating shadows of static objects..." );
			shadowMap = new CByteImage;
			if (!shadowMap->Allocate( iWidth,iHeight ))
			{
				delete shadowMap;
				return false;
			}
			shadowMap->Clear();
			float shadowAmmount = 255.0f*m_doc->GetLighting()->iShadowIntensity/100.0f;
			Vec3 sunVector = m_doc->GetLighting()->GetSunVector();

			GetIEditor()->GetVegetationMap()->GenerateShadowMap( *shadowMap,shadowAmmount,sunVector );
		}
		
		CLogFile::WriteLine("Generating Terrain Lighting..." );
		GetIEditor()->SetStatusText( "Generating Terrain Lighting..." );


		// Calculate the lighting. Fucntion will also use pLightingBits if present
//		cLighting.LightArray16(iWidth, iHeight, pSurface, pHeightmapData, 
//			m_doc->GetLighting(), pLightingBits,shadowMap );

		if (shadowMap)
			delete shadowMap;

		int t2 = GetTickCount();
		CLogFile::FormatLine( "Texture lighted in %dms",t2-t1 );
	}

	// After lighting add Colored Water layer.
	if (bShowWaterColor)
	{
		// Apply water level.
		// Add a temporary water layer to the list
		CLayer *water = new CLayer;
		//water->LoadTexture(MAKEINTRESOURCE(IDB_WATER), 128, 128);
		water->FillWithColor( m_doc->GetWaterColor(),128,128 );
		water->GenerateWaterLayer16(pHeightmapData,iWidth, iHeight, waterLevel );

		// Set the write pointer (will be incremented) for the surface data
		DWORD *pTex = pSurface;
		
		uint32 layerWidth = water->GetTextureWidth();
		uint32 layerHeight = water->GetTextureHeight();
		
		// Draw the first layer, without layer mask.
		for (iTexY=0; iTexY < iHeight; iTexY++)
		{
			uint32 layerY = iTexY % layerHeight;
			for (iTexX=0; iTexX < iWidth; iTexX++)
			{
				// Get the color of the tiling texture at this position
				if (water->GetLayerMaskPoint(iTexX,iTexY) > 0)
				{
					*pTex = water->GetTexturePixel( iTexX % layerWidth,layerY );
				}
				pTex++;
			}
		}
		delete water;
	}


	if (bConvertToABGR)
	{
		GetIEditor()->SetStatusText( "Convert surface texture to ABGR..." );
		// Set the write pointer (will be incremented) for the surface data
		pTextureDataWrite = pSurface;
		for (iTexY=0; iTexY<(int) iHeight; iTexY++)
		{
			for (iTexX=0; iTexX<(int) iWidth; iTexX++)
			{
				*pTextureDataWrite++ = ((* pTextureDataWrite & 0x00FF0000) >> 16) |
																(* pTextureDataWrite & 0x0000FF00) |
																((* pTextureDataWrite & 0x000000FF) << 16);
			}
		}
	}


	////////////////////////////////////////////////////////////////////////
	// Finished
	////////////////////////////////////////////////////////////////////////

	// Should we return or free the heightmap data ?
	if (ppHeightmapData)
	{
		*ppHeightmapData = pHeightmapData;
		pHeightmapData = NULL;
	}
	else
	{
		// Free the heightmap data
		delete [] pHeightmapData;
		pHeightmapData = NULL;
	}

	// We are finished with the calculations
	EndWaitCursor();
	GetIEditor()->SetStatusText("Ready");

	int t2 = GetTickCount();
	CLogFile::FormatLine( "Texture surface generate in %dms",t2-t0 );

	return true;
}
*/


void CTerrainTextureDialog::OnImport() 
{
	////////////////////////////////////////////////////////////////////////
	// Import layer settings from a file
	////////////////////////////////////////////////////////////////////////

	char szFilters[] = "Layer Files (*.lay)|*.lay||";
	CAutoDirectoryRestoreFileDialog dlg(TRUE, "lay", "*.lay", OFN_EXPLORER|OFN_ENABLESIZING|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR, szFilters);
	CFile cFile;

	if (dlg.DoModal() == IDOK) 
	{
		CLogFile::FormatLine("Importing layer settings from %s", dlg.GetPathName().GetBuffer(0));

		CUndo undo("Import Texture Layers");
		GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );
		
		CXmlArchive ar;
		if( ar.Load(dlg.GetPathName()) )
		{
			CTerrainManager::GetTerrainManager().SerializeSurfaceTypes(ar);
			CTerrainManager::GetTerrainManager().SerializeLayerSettings(ar);
		}

		// Load the layers into the dialog
		ReloadLayerList();
	}
}


void CTerrainTextureDialog::OnRefineTerrainTextureTiles()
{
	if(AfxMessageBox("Refine TerrainTexture?\r\n"
		"(all terrain texture tiles become split in 4 parts so a tile with 2048x2048\r\n"
		"no longer limits the resolution) You need to save afterwards!",MB_YESNO)==IDYES)
		if(!GetIEditor()->GetHeightmap()->m_TerrainRGBTexture.RefineTiles())
			AfxMessageBox("TerrainTexture refine failed (make sure current data is saved)",MB_OK|MB_ICONHAND);
		else
			AfxMessageBox("Successfully refined TerrainTexture - Save is now required!",MB_OK);
}

void CTerrainTextureDialog::OnExport() 
{
	////////////////////////////////////////////////////////////////////////
	// Export layer settings to a file
	////////////////////////////////////////////////////////////////////////

	char szFilters[] = "Layer Files (*.lay)|*.lay||";
	CAutoDirectoryRestoreFileDialog dlg(FALSE, "lay", "ExportedLayers.lay", OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR, szFilters);
	CFile cFile;

	if (dlg.DoModal() == IDOK) 
	{
		CLogFile::FormatLine("Exporting layer settings to %s", dlg.GetPathName().GetBuffer(0));

		CXmlArchive ar( "LayerSettings" );
		CTerrainManager::GetTerrainManager().SerializeSurfaceTypes(ar);
		CTerrainManager::GetTerrainManager().SerializeLayerSettings(ar);
		ar.Save( dlg.GetPathName() );
	}
}

void CTerrainTextureDialog::OnApplyLighting()
{
	////////////////////////////////////////////////////////////////////////
	// Toggle between the on / off for the apply lighting state
	////////////////////////////////////////////////////////////////////////

	m_bUseLighting = m_bUseLighting ? false : true;
	GetMenu()->GetSubMenu(2)->CheckMenuItem(ID_PREVIEW_APPLYLIGHTING, 
		m_bUseLighting ? MF_CHECKED : MF_UNCHECKED);

	OnGeneratePreview();
}

void CTerrainTextureDialog::OnShowWater()
{
	////////////////////////////////////////////////////////////////////////
	// Toggle between the on / off for the show water state
	////////////////////////////////////////////////////////////////////////

	m_bShowWater = m_bShowWater ? false : true;
	GetMenu()->GetSubMenu(2)->CheckMenuItem(ID_PREVIEW_SHOWWATER, 
		m_bShowWater ? MF_CHECKED : MF_UNCHECKED);

	OnGeneratePreview();
}

void CTerrainTextureDialog::OnSetWaterLevel()
{
	////////////////////////////////////////////////////////////////////////
	// Let the user change the current water level
	////////////////////////////////////////////////////////////////////////

	// the dialog
	CNumberDlg cDialog( this,GetIEditor()->GetHeightmap()->GetWaterLevel(),"Set Water Level" );

	// Show the dialog
	if (cDialog.DoModal() == IDOK)
	{
		// Retrive the new water level from the dialog and save it in the document
		GetIEditor()->GetHeightmap()->SetWaterLevel(cDialog.GetValue());

		// We modified the document
		GetIEditor()->SetModifiedFlag();
		GetIEditor()->SetModifiedModule(eModifiedTerrain);

		OnGeneratePreview();
	}
}

void CTerrainTextureDialog::OnOptionsSetLayerBlending() 
{
	////////////////////////////////////////////////////////////////////////
	// Let the user change the current layer blending factor
	////////////////////////////////////////////////////////////////////////

	/*
	// Get the layer blending factor from the document and set it as default 
	// into the dialog
	CNumberDlg cDialog( this,m_doc->GetTerrainLayerBlendingFactor(),"Set Layer Blending" );

	// Show the dialog
	if (cDialog.DoModal() == IDOK)
	{
		// Retrieve the new layer blending factor from the dialog and
		// save it in the document
		m_doc->SetTerrainLayerBlendingFactor( cDialog.GetValue() );

		// We modified the document
		m_doc->SetModifiedFlag();
		m_doc->SetModifiedModule(eModifiedTerrain);

		OnGeneratePreview();
	}
	*/
}

void CTerrainTextureDialog::OnLayerExportTexture() 
{
	////////////////////////////////////////////////////////////////////////
	// Export the texture data, which is associated with the current layer,
	// to a bitmap file
	////////////////////////////////////////////////////////////////////////
	if (!m_pCurrentLayer)
		return;

	char szFilters[] = SUPPORTED_IMAGES_FILTER_SAVE;
	CAutoDirectoryRestoreFileDialog dlg(FALSE, "bmp",NULL, OFN_EXPLORER|OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR, szFilters);
	CFile cFile;

	// Does the current layer have texture data ?
	if (!m_pCurrentLayer->HasTexture())
	{
		AfxMessageBox("Current layer does no have a texture, can't export !");
		return;
	}

	if (dlg.DoModal() == IDOK) 
	{
		BeginWaitCursor();
		// Tell the layer to export its texture
		m_pCurrentLayer->ExportTexture( dlg.GetPathName() );
		EndWaitCursor();
	}
}

void CTerrainTextureDialog::OnHold() 
{
	////////////////////////////////////////////////////////////////////////
	// Make a temporary save of the current layer state
	////////////////////////////////////////////////////////////////////////

	CFile		cFile;
	CString	strFilename(gSettings.strStandardTempDirectory);

	Path::AddBackslash(strFilename);
	strFilename+="HoldStateTemp.lay";

	CFileUtil::CreateDirectory( gSettings.strStandardTempDirectory.GetBuffer() );
	CXmlArchive ar( "LayerSettings" );
	CTerrainManager::GetTerrainManager().SerializeLayerSettings(ar);
	ar.Save(strFilename);
}

void CTerrainTextureDialog::OnFetch() 
{
	////////////////////////////////////////////////////////////////////////
	// Load a previous save of the layer state
	////////////////////////////////////////////////////////////////////////

	CFile cFile;

	CString	strFilename(gSettings.strStandardTempDirectory);

	Path::AddBackslash(strFilename);
	strFilename+="HoldStateTemp.lay";


	if (!PathFileExists(strFilename.GetBuffer()))
	{
		AfxMessageBox("You have to use 'Hold' before using 'Fetch' !");
		return;
	}

	// Does the document contain unsaved data ?
	if (m_doc->IsModified())
	{
		if (AfxMessageBox("The document contains unsaved data, really fetch old state ?", 
			MB_ICONQUESTION | MB_YESNO, NULL) != IDYES)
		{
			return;
		}
	}

	CUndo undo("Fetch Layers");
	GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

	CXmlArchive ar;
	ar.Load( strFilename.GetBuffer() );
	CTerrainManager::GetTerrainManager().SerializeLayerSettings(ar);

	// Load the layers into the dialog
	ReloadLayerList();
}

void CTerrainTextureDialog::OnUseLayer() 
{
	////////////////////////////////////////////////////////////////////////
	// Click on the 'Use' checkbox of the current layer
	////////////////////////////////////////////////////////////////////////
	if (!m_pCurrentLayer)
		return;

	CButton ctrlButton;

	ASSERT(!IsBadReadPtr(m_pCurrentLayer, sizeof(CLayer)));

	// Change the internal in use value of the selected layer
	VERIFY(ctrlButton.Attach(GetDlgItem(IDC_USE_LAYER)->m_hWnd));
	m_pCurrentLayer->SetInUse((ctrlButton.GetCheck() == 1) ? true : false);
	ctrlButton.Detach();

	// Regenerate the preview
	OnGeneratePreview();
}

void CTerrainTextureDialog::OnAutoGenMask() 
{
	if (!m_pCurrentLayer)
		return;

	CUndo undo("Layer Autogen");
	GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

	m_pCurrentLayer->SetAutoGen( !m_pCurrentLayer->IsAutoGen() );
	UpdateControlData();

	// Regenerate the preview
	OnGeneratePreview();	
}

void CTerrainTextureDialog::OnLoadMask() 
{
	if (!m_pCurrentLayer)
		return;

	CString file;
	if (CFileUtil::SelectSingleFile( EFILE_TYPE_TEXTURE,file ))
	{
		CUndo undo("Load Layer Mask");
		GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

		// Load the texture
		if (!m_pCurrentLayer->LoadMask( file ))
			AfxMessageBox("Error while loading the texture !");

		// Update the texture information files
		UpdateControlData();
	}

	// Regenerate the preview
	OnGeneratePreview();
}

void CTerrainTextureDialog::OnExportMask() 
{
	if (!m_pCurrentLayer)
		return;

	// Export current layer mask to bitmap.
	CString filename;
	if (CFileUtil::SelectSaveFile( SUPPORTED_IMAGES_FILTER,"bmp","",filename ))
	{
		// Tell the layer to export its mask.
		m_pCurrentLayer->ExportMask( filename );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnLayersNewItem()
{
	CUndo undo("New Terrain Layer");
	GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

	// Add the layer
	CLayer *pNewLayer = new CLayer;
	pNewLayer->SetLayerName( "NewLayer" );
	pNewLayer->LoadTexture( CString(PathUtil::GetGameFolder() + "Textures/Defaults/defaultnouvs.dds") );
	pNewLayer->AssignMaterial("Materials/material_terrain_default");
  pNewLayer->GetOrRequestLayerId();

	CTerrainManager::GetTerrainManager().AddLayer( pNewLayer );

	ReloadLayerList();

	SelectLayer( pNewLayer );

	// Update the controls with the data from the layer
	UpdateControlData();

	// Regenerate the preview
	OnGeneratePreview();

	m_bIgnoreNoitify = true;
	GetIEditor()->Notify(eNotify_OnInvalidateControls);
	m_bIgnoreNoitify = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnLayersDeleteItem()
{
	if(!m_pCurrentLayer)
	{
		MessageBox( "No target layers selected" );
		return;
	}

	string	strWarningMessage;
	CString strLayerName=m_pCurrentLayer->GetLayerName();

	strWarningMessage.Format("Are you sure you want to delete layer %s?",strLayerName.GetBuffer());

	if (MessageBox(strWarningMessage.c_str(),"Confirm delete layer",MB_YESNO)==IDYES)
	{	
	CUndo undo("Delete Terrain Layer");
	GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

	/*
	// Ask before removing the layer
	int nResult;
	nResult = MessageBox("Do you really want to remove the selected layer ?" , "Remove Layer", MB_ICONQUESTION | 
		MB_YESNO | MB_APPLMODAL | MB_TOPMOST);
	if (nResult != IDYES)
		return;
	*/

	CLayer *pLayerToDelete = m_pCurrentLayer;

	SelectLayer(0,true);

		// Find the layer inside the layer list in the document and remove it.
		CTerrainManager::GetTerrainManager().RemoveLayer( pLayerToDelete );

	ReloadLayerList();

		// Regenerate the preview
		OnGeneratePreview();
	}

	m_bIgnoreNoitify = true;
	GetIEditor()->Notify(eNotify_OnInvalidateControls);
	m_bIgnoreNoitify = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnLayersMoveItemUp()
{
	CUndo undo("Move Terrain Layer Up");
	GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

	if (!m_pCurrentLayer)
		return;

	CLayer *pLayer = m_pCurrentLayer;

	int nIndexCur = -1;
	for (int i = 0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
	{
		if (CTerrainManager::GetTerrainManager().GetLayer(i) == m_pCurrentLayer)
		{
			nIndexCur = i;
			break;
		}
	}

	if (nIndexCur < 1)
		return;

	CTerrainManager::GetTerrainManager().SwapLayers( nIndexCur,nIndexCur-1 );

	ReloadLayerList();

	SelectLayer( pLayer,true );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnLayersMoveItemDown()
{
	CUndo undo("Move Terrain Layer Down");
	GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

	if (!m_pCurrentLayer)
		return;

	CLayer *pLayer = m_pCurrentLayer;

	int nIndexCur = -1;
	for (int i = 0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
	{
		if (CTerrainManager::GetTerrainManager().GetLayer(i) == m_pCurrentLayer)
		{
			nIndexCur = i;
			break;
		}
	}

	if (nIndexCur < 0 || nIndexCur >= CTerrainManager::GetTerrainManager().GetLayerCount()-1)
		return;

	CTerrainManager::GetTerrainManager().SwapLayers( nIndexCur,nIndexCur+1 );

	ReloadLayerList();

	SelectLayer( pLayer,true );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::GeneratePreviewImageList()
{
	if (m_imageList.GetSafeHandle())
		m_imageList.DeleteImageList();
	VERIFY( m_imageList.Create( TEXTURE_PREVIEW_SIZE,TEXTURE_PREVIEW_SIZE,ILC_COLOR32,0,1 ) );

	// Allocate the memory for the texture
	CImage preview;
	preview.Allocate( TEXTURE_PREVIEW_SIZE,TEXTURE_PREVIEW_SIZE );
	preview.Fill(128);

	CBitmap bmp;
	VERIFY(bmp.CreateBitmap(TEXTURE_PREVIEW_SIZE,	TEXTURE_PREVIEW_SIZE, 1, 32, NULL));

	for (int i = 0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
	{
		CLayer *pLayer = CTerrainManager::GetTerrainManager().GetLayer(i);

		CImage &previewImage = pLayer->GetTexturePreviewImage();
		if (previewImage.IsValid())
			CImageUtil::ScaleToFit( previewImage,preview );
		preview.SwapRedAndBlue();

		// Write the texture data into the bitmap
		bmp.SetBitmapBits( preview.GetSize(),(DWORD*)preview.GetData() );

		int nRes = m_imageList.Add( &bmp,RGB(255,0,255) );
	}

	m_wndReport.SetImageList( &m_imageList );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnSize( UINT nType, int cx, int cy )
{
	__super::OnSize(nType,cx,cy);
	RecalcLayout();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::RecalcLayout()
{
	if (m_wndTaskPanel.m_hWnd)
	{
		CRect rc;
		GetClientRect(rc);

		rc.top += 30;

		CRect rcTask(rc);
		rcTask.right = 200;
		rc.left = rcTask.right+1;
		m_wndTaskPanel.MoveWindow(rcTask);

		CRect rcReport(rc);
		m_wndReport.MoveWindow(rcReport);
	}
}

//////////////////////////////////////////////////////////////////////////
LRESULT CTerrainTextureDialog::OnTaskPanelNotify( WPARAM wParam, LPARAM lParam )
{
	switch(wParam) {
	case XTP_TPN_CLICK:
		{
			CXTPTaskPanelGroupItem* pItem = (CXTPTaskPanelGroupItem*)lParam;
			UINT nCmdID = pItem->GetID();
			switch (nCmdID)
			{
			case CMD_LAYER_ADD:
				OnLayersNewItem();
				break;
			case CMD_LAYER_DELETE:
				OnLayersDeleteItem();
				break;
			case CMD_LAYER_MOVEUP:
				OnLayersMoveItemUp();
				break;
			case CMD_LAYER_MOVEDOWN:
				OnLayersMoveItemDown();
				break;
			case CMD_LAYER_LOAD_TEXTURE:
				OnLoadTexture();
				break;
			case CMD_LAYER_ASIGN_MATERIAL:
				OnAssignMaterial();
				break;
			case CMD_OPEN_MATERIAL_EDITOR:
				GetIEditor()->OpenView( "Material Editor" );
				break;
			}
		}
		break;

	case XTP_TPN_RCLICK:
		break;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainTextureDialog::GetSelectedLayers( Layers &layers )
{
	layers.clear();
	POSITION pos = m_wndReport.GetSelectedRows()->GetFirstSelectedRowPosition();
	while (pos)
	{
		CTerrainLayerRecord *pRec = DYNAMIC_DOWNCAST(CTerrainLayerRecord,m_wndReport.GetSelectedRows()->GetNextSelectedRow( pos )->GetRecord());
		if (pRec && pRec->m_pLayer)
			layers.push_back(pRec->m_pLayer);
	}
	return !layers.empty();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnReportSelChange( NMHDR * pNotifyStruct, LRESULT *result )
{
	CLayer *pLayer = NULL;

	Layers layers;
	if (GetSelectedLayers(layers))
		pLayer = layers[0];
	
	if (pLayer != m_pCurrentLayer)
		SelectLayer( pLayer,true );

	*result = 0;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnReportKeyDown(NMHDR *pNMHDR, LRESULT *pResult)
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNMHDR;
	*pResult = 0;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnReportClick(NMHDR * pNotifyStruct, LRESULT *result )
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;
	if (pItemNotify->pColumn && pItemNotify->pColumn->GetIndex() == 0)
	{
		
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnReportRClick(NMHDR * pNotifyStruct, LRESULT *result )
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;

	*result = 0;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnReportHyperlink(NMHDR * pNotifyStruct, LRESULT * result)
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;
	if (pItemNotify->nHyperlink >= 0)
	{
		CTerrainLayerRecord* pRecord = DYNAMIC_DOWNCAST(CTerrainLayerRecord, pItemNotify->pRow->GetRecord());
		if (pRecord)
		{
			if (pItemNotify->pColumn->GetIndex() == COLUMN_MATERIAL)
			{
				CString mtlName;
				CSurfaceType *pSurfaceType = pRecord->m_pLayer->GetSurfaceType();
				if (pSurfaceType)
				{
					mtlName = pSurfaceType->GetMaterial();
				}

				IMaterial* pMtl = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(mtlName,false);
				if (pMtl)
				{
					GetIEditor()->GetMaterialManager()->GotoMaterial( pMtl );
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::SelectLayer( CLayer *pLayer,bool bSelectUI )
{
	// Unselect all layers.
	for (int i = 0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
	{
		CTerrainManager::GetTerrainManager().GetLayer(i)->SetSelected(false);
	}

	// Set it as the current one
	m_pCurrentLayer = pLayer;

	if (m_pCurrentLayer)
		m_pCurrentLayer->SetSelected(true);

	//	m_bMaskPreviewValid = false;

	if (bSelectUI)
	{
		m_wndReport.GetSelectedRows()->Clear();
		// Get the layer associated with the selection
		int count = m_wndReport.GetRows()->GetCount();
		for (int i = 0; i < count; i++)
		{
			CTerrainLayerRecord *pRec = DYNAMIC_DOWNCAST(CTerrainLayerRecord,m_wndReport.GetRows()->GetAt(i)->GetRecord());
			if (pRec && pRec->m_pLayer && pRec->m_pLayer->IsSelected())
			{
				m_wndReport.GetSelectedRows()->Add( m_wndReport.GetRows()->GetAt(i) );
				break;
			}
		}

		// Update the controls with the data from the layer
		UpdateControlData();
	}

	m_bIgnoreNoitify = true;
	GetIEditor()->Notify(eNotify_OnSelectionChange);
	m_bIgnoreNoitify = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnReportPropertyChanged(NMHDR * pNotifyStruct, LRESULT * /*result*/)
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*) pNotifyStruct;
	///	

	CUndo undo("Texture Layer Change");
	GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

	CTerrainLayerRecord *pRec = DYNAMIC_DOWNCAST(CTerrainLayerRecord,pItemNotify->pItem->GetRecord());
	if (!pRec)
		return;
	CLayer *pLayer = pRec->m_pLayer;
	if (!pLayer)
		return;

	if (pItemNotify->pColumn->GetIndex() == COLUMN_LAYER_NAME)
	{
		CString newLayerName = ((CXTPReportRecordItemText*)pItemNotify->pItem)->GetValue();
		pRec->m_pLayer->SetLayerName( newLayerName );
	}
	if (pItemNotify->pColumn->GetIndex() == COLUMN_MIN_HEIGHT)
	{
		double value = ((CXTPReportRecordItemNumber*)pItemNotify->pItem)->GetValue();
		pRec->m_pLayer->SetLayerStart(value);
	}
	if (pItemNotify->pColumn->GetIndex() == COLUMN_MAX_HEIGHT)
	{
		double value = ((CXTPReportRecordItemNumber*)pItemNotify->pItem)->GetValue();
		pRec->m_pLayer->SetLayerEnd(value);
	}
	if (pItemNotify->pColumn->GetIndex() == COLUMN_MIN_ANGLE)
	{
		double value = ((CXTPReportRecordItemNumber*)pItemNotify->pItem)->GetValue();
		pRec->m_pLayer->SetLayerMinSlopeAngle(value);
	}
	if (pItemNotify->pColumn->GetIndex() == COLUMN_MAX_ANGLE)
	{
		double value = ((CXTPReportRecordItemNumber*)pItemNotify->pItem)->GetValue();
		pRec->m_pLayer->SetLayerMaxSlopeAngle(value);
	}
  if (pItemNotify->pColumn->GetIndex() == COLUMN_BRIGHTNESS)
  {
    double value = ((CXTPReportRecordItemNumber*)pItemNotify->pItem)->GetValue();
    pRec->m_pLayer->SetLayerBrightness(value);
  }
  if (pItemNotify->pColumn->GetIndex() == COLUMN_TILING)
  {
    double value = ((CXTPReportRecordItemNumber*)pItemNotify->pItem)->GetValue();
    pRec->m_pLayer->SetLayerTiling(value);
  }
  if (pItemNotify->pColumn->GetIndex() == COLUMN_SORT_ORDER)
  {
    double value = ((CXTPReportRecordItemNumber*)pItemNotify->pItem)->GetValue();
    pRec->m_pLayer->SetLayerSortOrder(value);
  }
  if (pItemNotify->pColumn->GetIndex() == COLUMN_SPEC_AMOUNT)
  {
    double value = ((CXTPReportRecordItemNumber*)pItemNotify->pItem)->GetValue();
    pRec->m_pLayer->SetLayerSpecularAmount(value);
  }
  if (pItemNotify->pColumn->GetIndex() == COLUMN_USE_REMESH)
  {
    double value = ((CXTPReportRecordItemNumber*)pItemNotify->pItem)->GetValue();
    pRec->m_pLayer->SetLayerUseRemeshing(value);
  }

	m_bIgnoreNoitify = true;
	GetIEditor()->Notify(eNotify_OnInvalidateControls);
	m_bIgnoreNoitify = false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnShowPreviewCheckbox()
{
	ReloadLayerList();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnAssignMaterial()
{
	Layers layers;
	if (GetSelectedLayers(layers))
	{
		CUndo undo("Assign Layer Material");
		GetIEditor()->RecordUndo( new CTerrainLayersUndoObject );

		CMaterial *pMaterial = GetIEditor()->GetMaterialManager()->GetCurrentMaterial();
		assert( pMaterial != NULL );
		if ( pMaterial == NULL )
		{
			return;
		}

		for (int i = 0; i < layers.size(); i++)
		{
			layers[i]->AssignMaterial( pMaterial->GetName() );
		}
	}
	else
	{
		MessageBox( "No target layers selected" );
	}
	ReloadLayerList();

	CTerrainManager::GetTerrainManager().ReloadSurfaceTypes();
}


//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::PostNcDestroy()
{
  delete this;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnDataBaseItemEvent( IDataBaseItem* pItem, EDataBaseItemEvent event )
{
	if ( event == EDB_ITEM_EVENT_SELECTED )
	{
		UpdateAssignMaterialItem();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::UpdateAssignMaterialItem()
{
	if ( m_pAssignMaterialLink == NULL )
	{
		return;
	}

	CMaterial* pMaterial = GetIEditor()->GetMaterialManager()->GetCurrentMaterial();
	bool materialSelected = ( pMaterial != NULL );

	m_pAssignMaterialLink->SetEnabled( materialSelected );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainTextureDialog::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	if(m_bIgnoreNoitify)
			return;
	switch (event)
	{
	case eNotify_OnBeginNewScene:
	case eNotify_OnBeginSceneOpen:
		ClearData();
		break;
	case eNotify_OnEndNewScene:
	case eNotify_OnEndSceneOpen:
		ReloadLayerList();
		break;

	case eNotify_OnSelectionChange:
		{
      static int nLastSelection = -1;
			CCryEditDoc *pDoc = GetIEditor()->GetDocument();
			for (int i = 0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
			{
				if(CTerrainManager::GetTerrainManager().GetLayer(i)->IsSelected())
				{
          if(nLastSelection != i)
          {
            SelectLayer( CTerrainManager::GetTerrainManager().GetLayer(i) );
            nLastSelection = i;
          }
					break;
				}
			}
		}
		break;

	case eNotify_OnInvalidateControls:
		{
			CLayer * pLayer = 0;
			CCryEditDoc *pDoc = GetIEditor()->GetDocument();
			for (int i = 0; i < CTerrainManager::GetTerrainManager().GetLayerCount(); i++)
			{
				if(CTerrainManager::GetTerrainManager().GetLayer(i)->IsSelected())
				{
					pLayer = CTerrainManager::GetTerrainManager().GetLayer(i);
					break;
				}
			}

			if(pLayer)
			{
				int count = m_wndReport.GetRows()->GetCount();
				for (int i = 0; i < count; i++)
				{
					CTerrainLayerRecord *pRec = DYNAMIC_DOWNCAST(CTerrainLayerRecord,m_wndReport.GetRows()->GetAt(i)->GetRecord());
					if (pRec && pRec->m_pLayer == pLayer)
					{
						((CXTPReportRecordItemNumber * )(pRec->GetItem(COLUMN_MIN_HEIGHT)))->SetValue(pLayer->GetLayerStart());
						((CXTPReportRecordItemNumber * )(pRec->GetItem(COLUMN_MAX_HEIGHT)))->SetValue(pLayer->GetLayerEnd());
						((CXTPReportRecordItemNumber * )(pRec->GetItem(COLUMN_MIN_ANGLE)))->SetValue(pLayer->GetLayerMinSlopeAngle());
						((CXTPReportRecordItemNumber * )(pRec->GetItem(COLUMN_MAX_ANGLE)))->SetValue(pLayer->GetLayerMaxSlopeAngle());

						m_wndReport.GetSelectedRows()->Clear();
						// Get the layer associated with the selection
						int count = m_wndReport.GetRows()->GetCount();
						for (int i = 0; i < count; i++)
						{
							CTerrainLayerRecord *pRec = DYNAMIC_DOWNCAST(CTerrainLayerRecord,m_wndReport.GetRows()->GetAt(i)->GetRecord());
							if (pRec && pRec->m_pLayer && pRec->m_pLayer->IsSelected())
							{
								m_wndReport.GetSelectedRows()->Add( m_wndReport.GetRows()->GetAt(i) );
								break;
							}
						}
						break;
					}
				}
			}
		}
		break;
	}
}
