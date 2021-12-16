////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   TerrainMiniMapTool.cpp
//  Version:     v1.00
//  Created:     11/1/2002 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description: Terrain Mini Map Tool implementation.
// -------------------------------------------------------------------------
//  History:     3/3/2010 Refactoring by Sergiy Shaykin
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <I3DEngine.h>
#include "CryEditDoc.h"
#include "GameEngine.h"
#include "Util/ImageTIF.h"

#include "TerrainMiniMapTool.h"

#define MAP_SCREENSHOT_SETTINGS "MapScreenshotSettings.xml"

//////////////////////////////////////////////////////////////////////////
// class CUndoTerrainMiniMapTool
// Undo object for storing Mini Map states
//////////////////////////////////////////////////////////////////////////
class CUndoTerrainMiniMapTool : public IUndoObject
{
public:
	CUndoTerrainMiniMapTool( )
	{
		m_Undo = GetIEditor()->GetDocument()->GetCurrentMission()->GetMinimap();
	}
protected:
	virtual int GetSize() { return sizeof(*this); }
	virtual const char* GetDescription() { return "MiniMap Params"; };

	virtual void Undo( bool bUndo )
	{
		if (bUndo)
			m_Redo = GetIEditor()->GetDocument()->GetCurrentMission()->GetMinimap();
		GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_Undo);
		if (bUndo)
			GetIEditor()->Notify(eNotify_OnInvalidateControls);
	}
	virtual void Redo()
	{
		GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_Redo);
		GetIEditor()->Notify(eNotify_OnInvalidateControls);
	}
private:
	SMinimapInfo m_Undo;
	SMinimapInfo m_Redo;
};






//////////////////////////////////////////////////////////////////////////
// Panel.
//////////////////////////////////////////////////////////////////////////
class CTerrainMiniMapPanel : public CDialog
{
public:
	CSliderCtrl	m_radius;
	CTerrainMiniMapTool *m_tool;
	
	CCustomButton	m_btnGenerate;
	CCustomButton m_btnSave;
	CNumberCtrl m_cameraHeight;
	CComboBox m_resolutions;

	CTerrainMiniMapPanel(class CTerrainMiniMapTool *tool,CWnd* pParent = NULL) : CDialog(IDD_PANEL_TERRAIN_MINIMAP,pParent)
	{
		m_tool = tool;
		Create( IDD_PANEL_TERRAIN_MINIMAP,pParent );
	}

	void ReloadValues()
	{
		SMinimapInfo minimap = m_tool->GetMinimap();
		m_cameraHeight.SetValue( minimap.vExtends.x );
		m_resolutions.SetCurSel(-1);
		int nRes = 0;
		int nStart = 8;
		for (int i = 0; i < 10; i++)
		{
			if (minimap.textureWidth == (nStart << i))
			{
				m_resolutions.SetCurSel(i);
				break;
			}
		}
	}
	void InitPanel()
	{
		BOOL isEnable = GetIEditor()->Get3DEngine()->GetITerrain() ? TRUE : FALSE;
		int nIDs[] = {IDC_DDS_NAME, IDC_TIF_NAME, IDC_XML_NAME, IDC_IS_DDS, IDC_IS_TIF, IDC_GENERATE, IDC_STATIC_OUTPUT, IDC_STATIC_DDS, IDC_STATIC_TIF, IDC_FILE_SAVE_AS};
		for( int i=0; i<sizeof(nIDs)/sizeof(int); i++ )
			if(GetDlgItem( nIDs[i] ))
				GetDlgItem( nIDs[i] )->EnableWindow(isEnable);

		if(!GetIEditor()->Get3DEngine()->GetITerrain())
			return;

		CString levelName = Path::RemoveExtension( Path::GetFileName(GetIEditor()->GetGameEngine()->GetLevelName()) );
		SetDlgItemText( IDC_DDS_NAME, levelName );
		SetDlgItemText( IDC_TIF_NAME, levelName );

		SetDlgItemText( IDC_XML_NAME, m_tool->GetPath() + m_tool->GetFilename()+".xml" );

		((CButton*) GetDlgItem( IDC_IS_DDS ))->SetCheck(TRUE);
		((CButton*) GetDlgItem( IDC_IS_TIF ))->SetCheck(TRUE);
	}

protected:
	DECLARE_MESSAGE_MAP()

	virtual void OnOK() {};
	virtual void OnCancel() {};

	virtual BOOL OnInitDialog()
	{
		__super::OnInitDialog();

		int nStart = 8;
		for (int i = 0; i < 12; i++)
		{
			char str[8];
			sprintf( str,"%d",nStart<<i );
			m_resolutions.AddString( str );
		}
		m_cameraHeight.Create(this,IDC_HEIGHT,0);

		SetDlgItemText( IDC_SCRIPT_NAME, MAP_SCREENSHOT_SETTINGS );

		ReloadValues();
		InitPanel();

		return TRUE;  // return TRUE unless you set the focus to a control
	}
	virtual void DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_GENERATE, m_btnGenerate);
		DDX_Control(pDX, IDC_RESOLUTION,m_resolutions );
	}
	
	afx_msg void OnGenerateArea()
	{
		m_tool->Generate();
	}
	afx_msg void OnHeightChange()
	{
		m_tool->SetCameraHeight(m_cameraHeight.GetValue());
	}
	afx_msg void OnResolutionChange()
	{
		int nSel = m_resolutions.GetCurSel();
		if (nSel >=0 )
		{
			int nRes = 0;
			int nStart = 8;
			for (int i = 0; i < 12; i++)
			{
				if (i == nSel)
				{
					m_tool->SetResolution(nStart << i);
					break;
				}
			}
		}
	}
	afx_msg void OnFileCommands()
	{
		CFileUtil::PopupMenu(MAP_SCREENSHOT_SETTINGS, Path::MakeFullPath("Editor"), this);
	}
	afx_msg void OnFileSaveAs()
	{
		CString path = m_tool->GetPath();
		CString filename = m_tool->GetFilename() + ".xml";
		
		if (CFileUtil::SelectSaveFile( "XML Files (*.xml)|*.xml" , "xml", path, filename ))
		{
			path = Path::GetPath(filename);
			filename = Path::RemoveExtension( Path::GetFileName(filename));
			m_tool->SetPath(path);
			m_tool->SetFilename(filename);
			SetDlgItemText( IDC_XML_NAME, m_tool->GetPath() + m_tool->GetFilename() +".xml" );
		}
	}
};

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CTerrainMiniMapPanel, CDialog)
	ON_BN_CLICKED(IDC_GENERATE, OnGenerateArea)
	ON_EN_CHANGE(IDC_HEIGHT,OnHeightChange)
	ON_CBN_SELENDOK(IDC_RESOLUTION,OnResolutionChange)
	ON_BN_CLICKED(IDC_FILE_COMMANDS, OnFileCommands)
	ON_BN_CLICKED(IDC_FILE_SAVE_AS, OnFileSaveAs)
END_MESSAGE_MAP()


namespace
{
	int s_panelId = 0;
	CTerrainMiniMapPanel* s_panel = 0;
}





//////////////////////////////////////////////////////////////////////////
// CTerrainMiniMapTool
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CTerrainMiniMapTool,CEditTool)


//////////////////////////////////////////////////////////////////////////
CTerrainMiniMapTool::CTerrainMiniMapTool()
{
	m_minimap = GetIEditor()->GetDocument()->GetCurrentMission()->GetMinimap();
	m_bDragging = false;
	b_stateScreenShot = false;

	m_path = Path::GamePathToFullPath(GetIEditor()->GetGameEngine()->GetLevelName());
	char rootpath[_MAX_PATH];
	GetCurrentDirectory(sizeof(rootpath),rootpath);
	m_path = Path::AddBackslash(CString(rootpath) + "\\" + Path::GamePathToFullPath( m_path ));
	m_filename = Path::RemoveExtension( Path::GetFileName(GetIEditor()->GetGameEngine()->GetLevelName()) );

	GetIEditor()->RegisterNotifyListener( this );
}

//////////////////////////////////////////////////////////////////////////
CTerrainMiniMapTool::~CTerrainMiniMapTool()
{
	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->CancelUndo();

	GetIEditor()->Get3DEngine()->SetScreenshotCallback(0);
	GetIEditor()->UnregisterNotifyListener( this );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::BeginEditParams( IEditor *ie,int flags )
{
	if (!s_panelId)
	{
		s_panel = new CTerrainMiniMapPanel(this,AfxGetMainWnd());
		s_panelId = GetIEditor()->AddRollUpPage( ROLLUP_TERRAIN,"Mini Map",s_panel );
		AfxGetMainWnd()->SetFocus();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::EndEditParams()
{
	if (s_panelId)
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_TERRAIN,s_panelId);
		s_panelId = 0;
		s_panel = 0;
	}
}

void CTerrainMiniMapTool::SetResolution(int nResolution)
{
	GetIEditor()->BeginUndo();
	if (CUndo::IsRecording())
		CUndo::Record( new CUndoTerrainMiniMapTool() );
	m_minimap.textureWidth = nResolution;
	m_minimap.textureHeight = nResolution;
	GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_minimap);
	GetIEditor()->AcceptUndo( "Mini Map Resolution" );
}

void CTerrainMiniMapTool::SetCameraHeight(float fHeight)
{
	GetIEditor()->BeginUndo();
	if (CUndo::IsRecording())
		CUndo::Record( new CUndoTerrainMiniMapTool() );
	m_minimap.vExtends = Vec2(fHeight, fHeight);
	GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_minimap);
	GetIEditor()->AcceptUndo( "Mini Map Camera Height" );
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainMiniMapTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	if (event == eMouseLDown || (event == eMouseMove && (flags&MK_LBUTTON)))
	{
		Vec3 pos = view->ViewToWorld( point,0,true );

		if (!m_bDragging)
		{
			GetIEditor()->BeginUndo();
			m_bDragging = true;
		}
		else
		{
			GetIEditor()->RestoreUndo();
		}

		if (CUndo::IsRecording())
			CUndo::Record( new CUndoTerrainMiniMapTool() );
			
		m_minimap.vCenter.x	=	pos.x;
		m_minimap.vCenter.y	=	pos.y;
		GetIEditor()->GetDocument()->GetCurrentMission()->SetMinimap(m_minimap);

		return true;
	}
	else
	{
		// Stop.
		if (m_bDragging)
		{
			m_bDragging = false;
			GetIEditor()->AcceptUndo( "Mini Map Position" );
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::Display( DisplayContext &dc )
{
	dc.SetColor( 0,0,1 );
	dc.DrawTerrainRect( m_minimap.vCenter.x-0.5f,m_minimap.vCenter.y-0.5f,
											m_minimap.vCenter.x+0.5f,m_minimap.vCenter.y+0.5f,
											1.0f	);
	dc.DrawTerrainRect( m_minimap.vCenter.x-5.f,m_minimap.vCenter.y-5.f,
											m_minimap.vCenter.x+5.f,m_minimap.vCenter.y+5.f,
											1.0f	);
	dc.DrawTerrainRect( m_minimap.vCenter.x-50.f,m_minimap.vCenter.y-50.f,
											m_minimap.vCenter.x+50.f,m_minimap.vCenter.y+50.f,
											1.0f	);

	dc.SetColor( 0,1,0 );
	dc.SetLineWidth( 2 );
	dc.DrawTerrainRect( m_minimap.vCenter.x-m_minimap.vExtends.x,m_minimap.vCenter.y-m_minimap.vExtends.y,
											m_minimap.vCenter.x+m_minimap.vExtends.x,m_minimap.vCenter.y+m_minimap.vExtends.y,1.1f	);
	dc.SetLineWidth( 0 );
}


void CTerrainMiniMapTool::SendParameters(void* data, uint32 width, uint32 height, f32 minx, f32 miny, f32 maxx, f32 maxy)
{
	CWaitCursor wait;
	CString levelName = Path::RemoveExtension( Path::GetFileName(GetIEditor()->GetGameEngine()->GetLevelName()) );
	CString dataFile = m_path + m_filename+".xml";
	CString imageTIFFile = m_path + levelName+".tif";
	CString imageDDSFile = m_path + levelName+".dds";
	CString imageDDSFileShort = levelName+".dds";

	if(s_panel)
	{
		s_panel->GetDlgItemText(IDC_DDS_NAME, imageDDSFile);
		imageDDSFileShort = imageDDSFile+".dds";
		imageDDSFile = m_path + imageDDSFile+".dds";
		s_panel->GetDlgItemText(IDC_TIF_NAME, imageTIFFile);
		imageTIFFile = m_path + imageTIFFile+".tif";
	}

	uint8* buf=(uint8*)data;

	CImage image;
	image.Allocate(width, height);

	if(s_panel && ((CButton*) s_panel->GetDlgItem( IDC_IS_DDS ))->GetCheck())
	{
		for(int y=0; y<height; y++)
			for(int x=0; x<width; x++)
				image.ValueAt(x, height-1 - y) = RGBA8(buf[x*3 + y*width*3], buf[x*3+1 + y*width*3], buf[x*3+2 + y*width*3], 255);

		GetIEditor()->GetRenderer()->WriteDDS((byte*)image.GetData(),width,height,4,imageDDSFile,eTF_DXT3,1);
	}

	if(s_panel && ((CButton*) s_panel->GetDlgItem( IDC_IS_TIF ))->GetCheck())
	{
		for(int y=0; y<height; y++)
			for(int x=0; x<width; x++)
				image.ValueAt(x, height-1 - y) = RGBA8(buf[x*3+2 + y*width*3], buf[x*3+1 + y*width*3], buf[x*3 + y*width*3], 255);

		CImageTIF imageTIF;
		imageTIF.Save(imageTIFFile, image);
	}

	XmlNodeRef dataNode = GetISystem()->LoadXmlFile(dataFile);
	if(!dataNode)
		dataNode = GetISystem()->CreateXmlNode("MetaData");
	XmlNodeRef map = dataNode->findChild("MiniMap");
	if(!map)
	{
		map = GetISystem()->CreateXmlNode("MiniMap");
		dataNode->addChild(map);
	}
	map->setAttr("Filename", imageDDSFileShort);
	map->setAttr("startX", minx);
	map->setAttr("startY", miny);
	map->setAttr("endX", maxx);
	map->setAttr("endY", maxy);

	dataNode->saveToFile(dataFile);
}


//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::Generate()
{
	m_ConstClearList.clear();

	GetIEditor()->SetConsoleVar( "e_ScreenShotWidth",m_minimap.textureWidth );
	GetIEditor()->SetConsoleVar( "e_screenShotHeight",m_minimap.textureHeight );
	GetIEditor()->SetConsoleVar( "e_ScreenShotMapCamHeight",100000.f );

	GetIEditor()->SetConsoleVar( "e_ScreenShotMapCenterX",m_minimap.vCenter.x );
	GetIEditor()->SetConsoleVar( "e_ScreenShotMapCenterY",m_minimap.vCenter.y );

	GetIEditor()->SetConsoleVar( "e_ScreenShotMapSizeX",m_minimap.vExtends.x );
	GetIEditor()->SetConsoleVar( "e_ScreenShotMapSizeY",m_minimap.vExtends.y );

	string path = Path::MakeFullPath("Editor");
	string filename = path + "\\" + MAP_SCREENSHOT_SETTINGS;
	XmlNodeRef root = GetISystem()->LoadXmlFile( filename );
	if(root)
	{
		for(int i=0,nChilds=root->getChildCount();i<nChilds;i++)
		{
			XmlNodeRef ChildNode = root->getChild(i);
			const char* pTagName	=	ChildNode->getTag();
			ICVar* pVar	=	gEnv->pConsole->GetCVar(pTagName);
			if(pVar)
			{
				m_ConstClearList[pTagName]	=	pVar->GetFVal();
				const char* pAttr	=	ChildNode->getAttr("value");
				string Value	=	pAttr;
				pVar->Set(Value.c_str());
			}
			else
			{
				CryWarning( VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[MiniMap: %s] Console variable %s does not exist", MAP_SCREENSHOT_SETTINGS, pTagName );
			}
		}
	}
	else
	{
		m_ConstClearList["r_HDRRendering"]	= gEnv->pConsole->GetCVar("r_HDRRendering")->GetFVal();
		m_ConstClearList["r_PostProcessEffects"]	=	gEnv->pConsole->GetCVar("r_PostProcessEffects")->GetFVal();
		m_ConstClearList["e_Lods"]	= gEnv->pConsole->GetCVar("e_Lods")->GetFVal();
		m_ConstClearList["e_ViewDistRatio"]	= gEnv->pConsole->GetCVar("e_ViewDistRatio")->GetFVal();
		m_ConstClearList["e_VegetationSpritesDistanceRatio"]	= gEnv->pConsole->GetCVar("e_VegetationSpritesDistanceRatio")->GetFVal();
		m_ConstClearList["e_ViewDistRatioVegetation"]	= gEnv->pConsole->GetCVar("e_ViewDistRatioVegetation")->GetFVal();
		m_ConstClearList["e_TerrainLodRatio"]	= gEnv->pConsole->GetCVar("e_TerrainLodRatio")->GetFVal();
		m_ConstClearList["e_ScreenShotQuality"]	=	gEnv->pConsole->GetCVar("e_ScreenShotQuality")->GetFVal();
    m_ConstClearList["e_Vegetation"]	= gEnv->pConsole->GetCVar("e_Vegetation")->GetFVal();

		gEnv->pConsole->GetCVar("r_HDRRendering")->Set(0);
		gEnv->pConsole->GetCVar("r_PostProcessEffects")->Set(0);
		gEnv->pConsole->GetCVar("e_ScreenShotQuality")->Set(0);
		gEnv->pConsole->GetCVar("e_ViewDistRatio")->Set(10000.f);
		gEnv->pConsole->GetCVar("e_VegetationSpritesDistanceRatio")->Set(20);
		gEnv->pConsole->GetCVar("e_ViewDistRatioVegetation")->Set(100.f);
		gEnv->pConsole->GetCVar("e_Lods")->Set(0);
		gEnv->pConsole->GetCVar("e_TerrainLodRatio")->Set(0.f);
		gEnv->pConsole->GetCVar("e_Vegetation")->Set(0);
	}
	gEnv->pConsole->GetCVar("e_ScreenShotQuality")->Set(0);

	GetIEditor()->Get3DEngine()->SetScreenshotCallback(this);

	GetIEditor()->SetConsoleVar( "e_ScreenShot",3 );

	b_stateScreenShot = true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMiniMapTool::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
		case eNotify_OnIdleUpdate:
		{
			if(b_stateScreenShot)
			{
				ICVar * pVar = gEnv->pConsole->GetCVar("e_ScreenShot");
				if(pVar && pVar->GetIVal()==0)
				{
					for(std::map<string,float>::iterator it=m_ConstClearList.begin();it!=m_ConstClearList.end();++it)
					{
						ICVar* pVar	=	gEnv->pConsole->GetCVar(it->first.c_str());
						if(pVar)
							pVar->Set(it->second);
					}
					m_ConstClearList.clear();

					b_stateScreenShot = false;
					GetIEditor()->Get3DEngine()->SetScreenshotCallback(0);
				}
			}
			break;
		}
		case eNotify_OnInvalidateControls:
			m_minimap = GetIEditor()->GetDocument()->GetCurrentMission()->GetMinimap();
			if(s_panel)
				s_panel->ReloadValues();
			break;
	}
}