// PanelDisplayRender.cpp : implementation file
//

#include "stdafx.h"
#include "PanelDisplayRender.h"
#include "DisplaySettings.h"
#include "Settings.h"

#define LEAVE_FLAGS (RENDER_FLAG_BBOX)

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayRender dialog


CPanelDisplayRender::CPanelDisplayRender(CWnd* pParent /*=NULL*/)
	: CDialog(CPanelDisplayRender::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPanelDisplayRender)
	m_roads = FALSE;
	m_decals = FALSE;
	m_detailTex = FALSE;
	m_fog = FALSE;
	m_shadowMaps = FALSE;
	m_skyBox = FALSE;
	m_staticObj = FALSE;
	m_terrain = FALSE;
	m_water = FALSE;
	m_detailObj = FALSE;
	m_particles = FALSE;
	m_clouds = FALSE;
	m_imposters = FALSE;
	m_voxels = FALSE;
	m_beams = FALSE;
	m_globalIllum = FALSE;
	m_alphablend = FALSE;
	
	m_dbg_frame_profile = FALSE;
	m_dbg_aidebugdraw = FALSE;
	m_dbg_aidebugdrawcover = FALSE;
	m_dbg_physicsDebugDraw = FALSE;
	m_dbg_memory_info = FALSE;
	m_dbg_mem_stats = FALSE;
	m_dbg_texture_meminfo = FALSE;
	m_dbg_renderer_profile_shaders = FALSE;
	m_dbg_renderer_overdraw = FALSE;
	m_dbg_renderer_resources = FALSE;
	m_dbg_debug_lights = FALSE;
	m_dbg_budget_monitoring = FALSE;
  m_dbg_renderer_show_shaders = FALSE;
	m_dbg_drawcalls = FALSE;

	m_boIsUpdatingValues=false;

	//}}AFX_DATA_INIT

	assert(GetCurrentDisplayRender()==NULL);
	GetCurrentDisplayRender()=this;

	SetupCallbacks();

	Create( IDD,pParent );

	GetIEditor()->RegisterNotifyListener(this);
}

CPanelDisplayRender::~CPanelDisplayRender()
{
	GetIEditor()->UnregisterNotifyListener(this);
	GetCurrentDisplayRender()=NULL;
}


void CPanelDisplayRender::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	// Render flags.
	DDX_Check(pDX, IDC_DISPLAY_BEACHES, m_roads);
	DDX_Check(pDX, IDC_DISPLAY_DECALS, m_decals);
	DDX_Check(pDX, IDC_DISPLAY_DETAILTEX, m_detailTex);
	DDX_Check(pDX, IDC_DISPLAY_FOG, m_fog);
	DDX_Check(pDX, IDC_DISPLAY_SHADOWMAPS, m_shadowMaps);
	DDX_Check(pDX, IDC_DISPLAY_SKYBOX, m_skyBox);
	DDX_Check(pDX, IDC_DISPLAY_STATICOBJ, m_staticObj);
	DDX_Check(pDX, IDC_DISPLAY_TERRAIN, m_terrain);
	DDX_Check(pDX, IDC_DISPLAY_WATER, m_water);
	DDX_Check(pDX, IDC_DISPLAY_DETAILOBJ, m_detailObj);
	DDX_Check(pDX, IDC_DISPLAY_PARTICLES, m_particles);
	DDX_Check(pDX, IDC_DISPLAY_CLOUDS, m_clouds);
	DDX_Check(pDX, IDC_DISPLAY_VOXEL, m_voxels);
	DDX_Check(pDX, IDC_DISPLAY_IMPOSTERS, m_imposters);
	DDX_Check(pDX, IDC_DISPLAY_BEAMS, m_beams);
	DDX_Check(pDX, IDC_DISPLAY_GLOBAL_ILLUMINATION, m_globalIllum);
	GetDlgItem(IDC_DISPLAY_GLOBAL_ILLUMINATION)->EnableWindow(m_shadowMaps);

	// Debug flags.
	DDX_Check(pDX, IDC_DISPLAY_PROFILE2, m_dbg_frame_profile);
	DDX_Check(pDX, IDC_DISPLAY_AIDEBUGDRAW, m_dbg_aidebugdraw);
	DDX_Check(pDX, IDC_DISPLAY_AIDEBUGDRAWCOVER, m_dbg_aidebugdrawcover);
	DDX_Check(pDX, IDC_MEM_INFO, m_dbg_memory_info);
	DDX_Check(pDX, IDC_MEM_STATS, m_dbg_mem_stats);
	DDX_Check(pDX, IDC_TEXMEM_STATS, m_dbg_texture_meminfo);
	DDX_Check(pDX, IDC_DISPLAY_PHYSICSDEBUGDRAW, m_dbg_physicsDebugDraw);
	DDX_Check(pDX, IDC_DISPLAY_PROFILESHADERS, m_dbg_renderer_profile_shaders);
	DDX_Check(pDX, IDC_DISPLAY_OVERDRAW, m_dbg_renderer_overdraw);
	DDX_Check(pDX, IDC_DISPLAY_RENDERERRESOURCES, m_dbg_renderer_resources);
	DDX_Check(pDX, IDC_DISPLAY_DEBUG_LIGHTS, m_dbg_debug_lights);
	DDX_Check(pDX, IDC_DISPLAY_BUDGET_MONITOR, m_dbg_budget_monitoring);
  DDX_Check(pDX, IDC_DISPLAY_SHOW_SHADERS, m_dbg_renderer_show_shaders);
	DDX_Check(pDX, IDC_DISPLAY_DRAWCALLS, m_dbg_drawcalls);

}


BEGIN_MESSAGE_MAP(CPanelDisplayRender, CDialog)
	ON_BN_CLICKED(IDC_DISPLAY_FOG, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_ALL, OnDisplayAll)
	ON_BN_CLICKED(IDC_DISPLAY_NONE, OnDisplayNone)
	ON_BN_CLICKED(IDC_DISPLAY_INVERT, OnDisplayInvert)
	ON_BN_CLICKED(IDC_FILL_MODE, OnFillMode)
	ON_BN_CLICKED(IDC_WIREFRAME_MODE, OnWireframeMode)
	ON_BN_CLICKED(IDC_POINT_MODE,OnPointMode)
	ON_BN_CLICKED(IDC_DISPLAY_SKYBOX, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_TERRAIN, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_WATER, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_BEACHES, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_SHADOWMAPS, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_DETAILTEX, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_STATICOBJ, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_DECALS, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_DETAILOBJ, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_PARTICLES, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_CLOUDS, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_VOXEL, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_IMPOSTERS, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_BEAMS, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_DISPLAY_GLOBAL_ILLUMINATION, OnChangeRenderFlag)
	ON_BN_CLICKED(IDC_HIDE_HELPERS, OnBnClickedHideHelpers)

	// Debug flags.
	ON_BN_CLICKED(IDC_MEM_INFO, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_PROFILE2, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_AIDEBUGDRAW, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_AIDEBUGDRAWCOVER, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_MEM_STATS, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_TEXMEM_STATS, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_PHYSICSDEBUGDRAW, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_PROFILESHADERS, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_OVERDRAW, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_RENDERERRESOURCES, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_DEBUG_LIGHTS, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_BUDGET_MONITOR, OnChangeDebugFlag)
  ON_BN_CLICKED(IDC_DISPLAY_SHOW_SHADERS, OnChangeDebugFlag)
	ON_BN_CLICKED(IDC_DISPLAY_DRAWCALLS, OnChangeDebugFlag)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayRender message handlers

BOOL CPanelDisplayRender::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetControls();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPanelDisplayRender::SetupCallbacks()
{
	RegisterChangeCallback("e_Roads",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_Decals",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_DetailTextures",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_TerrainDetailMaterials",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_Fog",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_Terrain",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_SkyBox",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_Shadows",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_Vegetation",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_WaterOcean",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_WaterVolumes",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_ProcVegetation",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_Particles",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_Clouds",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_Voxel",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_VegetationSprites",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("r_Beams",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("e_GI",&CPanelDisplayRender::OnDisplayOptionChanged);
	RegisterChangeCallback("r_wireframe",&CPanelDisplayRender::OnDisplayOptionChanged);


	// Debug variables.
	RegisterChangeCallback("ai_DebugDraw",&CPanelDisplayRender::OnDebugOptionChanged);
	RegisterChangeCallback("r_TexLog",&CPanelDisplayRender::OnDebugOptionChanged);
	RegisterChangeCallback("MemStats",&CPanelDisplayRender::OnDebugOptionChanged);
	RegisterChangeCallback("p_draw_helpers",&CPanelDisplayRender::OnDebugOptionChanged);
	RegisterChangeCallback("r_ProfileShaders",&CPanelDisplayRender::OnDebugOptionChanged);
	RegisterChangeCallback("r_MeasureOverdraw",&CPanelDisplayRender::OnDebugOptionChanged);
	RegisterChangeCallback("r_Stats",&CPanelDisplayRender::OnDebugOptionChanged);
	RegisterChangeCallback("sys_enable_budgetmonitoring",&CPanelDisplayRender::OnDebugOptionChanged);
	RegisterChangeCallback("Profile",&CPanelDisplayRender::OnDebugOptionChanged);
	RegisterChangeCallback("r_DebugLights",&CPanelDisplayRender::OnDebugOptionChanged);
}

void CPanelDisplayRender::SetControls()
{
	ICVar		*piVariable(NULL);

	piVariable=gEnv->pConsole->GetCVar("e_Roads");
	if (piVariable)
	{
		m_roads=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Decals");
	if (piVariable)
	{
		m_decals=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_DetailTextures");
	if (piVariable)
	{
		m_detailTex=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_TerrainDetailMaterials");
	if (piVariable)
	{
		m_detailTex|=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}	

	piVariable=gEnv->pConsole->GetCVar("e_Fog");
	if (piVariable)
	{
		m_fog=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Terrain");
	if (piVariable)
	{
		m_terrain=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_SkyBox");
	if (piVariable)
	{
		m_skyBox=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Shadows");
	if (piVariable)
	{
		m_shadowMaps=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Vegetation");
	if (piVariable)
	{
		m_staticObj=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_WaterOcean");
	if (piVariable)
	{
		m_water=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_WaterVolumes");
	if (piVariable)
	{
		m_water=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_ProcVegetation");
	if (piVariable)
	{
		m_detailObj=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Particles");
	if (piVariable)
	{
		m_particles=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Clouds");
	if (piVariable)
	{
		m_clouds=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Voxel");
	if (piVariable)
	{
		m_voxels=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_VegetationSprites");
	if (piVariable)
	{
		m_imposters=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_Beams");
	if (piVariable)
	{
		m_beams=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_GI");
	if (piVariable)
	{
		m_globalIllum=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	// Debug
	piVariable=gEnv->pConsole->GetCVar("ai_DebugDraw");
	if (piVariable)
	{
		m_dbg_aidebugdraw=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("ai_DebugDrawCover");
	if (piVariable)
	{
		m_dbg_aidebugdrawcover=(piVariable->GetIVal()>1)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_TexLog");
	if (piVariable)
	{
		m_dbg_texture_meminfo=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("MemStats");
	if (piVariable)
	{
		m_dbg_mem_stats=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("p_draw_helpers");
	if (piVariable)
	{
		m_dbg_physicsDebugDraw=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}	

	piVariable=gEnv->pConsole->GetCVar("r_ProfileShaders");
	if (piVariable)
	{
		m_dbg_renderer_profile_shaders=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_MeasureOverdraw");
	if (piVariable)
	{
		m_dbg_renderer_overdraw=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_Stats");
	if (piVariable)
	{
		m_dbg_renderer_resources=(piVariable->GetIVal()==1)?TRUE:FALSE;
    m_dbg_renderer_show_shaders=(piVariable->GetIVal()==2)?TRUE:FALSE;
		m_dbg_drawcalls=(piVariable->GetIVal()==6)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("sys_enable_budgetmonitoring");
	if (piVariable)
	{
		m_dbg_budget_monitoring=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("Profile");
	if (piVariable)
	{
		m_dbg_frame_profile=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_DebugLights");
	if (piVariable)
	{
		m_dbg_debug_lights=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}
	
	uint32 flags = GetIEditor()->GetDisplaySettings()->GetDebugFlags();
	m_dbg_memory_info = (flags&DBG_MEMINFO)?TRUE:FALSE;

	UpdateData(FALSE);

	if (!GetIEditor()->GetDisplaySettings()->IsDisplayHelpers())
	{
		CheckDlgButton( IDC_HIDE_HELPERS,BST_CHECKED );
	}
	else
	{
		CheckDlgButton( IDC_HIDE_HELPERS,BST_UNCHECKED );
	}

	OnDisplayOptionChanged();
	OnDebugOptionChanged();
}

void CPanelDisplayRender::OnChangeRenderFlag() 
{
	UpdateData(TRUE);

	UpdateDisplayOptions();
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnChangeDebugFlag() 
{
	UpdateData(TRUE);

	uint32 flags = GetIEditor()->GetDisplaySettings()->GetDebugFlags();
	flags = 0;
	flags |= (m_dbg_memory_info)?DBG_MEMINFO:0;
	GetIEditor()->GetDisplaySettings()->SetDebugFlags( flags );

	UpdateDebugOptions();
}


//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDisplayAll() 
{
	uint32 flags = GetIEditor()->GetDisplaySettings()->GetRenderFlags();
	flags |= 0xFFFFFFFF & ~(LEAVE_FLAGS);
	GetIEditor()->GetDisplaySettings()->SetRenderFlags( flags );

	m_roads = TRUE;
	m_decals = TRUE;
	m_detailTex = TRUE;
	m_fog = TRUE;
	m_shadowMaps = TRUE;
	m_skyBox = TRUE;
	m_staticObj = TRUE;
	m_terrain = TRUE;
	m_water = TRUE;
	m_detailObj = TRUE;
	m_particles = TRUE;
	m_clouds = TRUE;
	m_imposters = TRUE;
	m_voxels = TRUE;
	m_beams = TRUE;
	m_globalIllum = TRUE;
	m_alphablend = TRUE;

	UpdateDisplayOptions();
	SetControls();
}

void CPanelDisplayRender::OnDisplayNone() 
{
	uint32 flags = GetIEditor()->GetDisplaySettings()->GetRenderFlags();
	flags &= (LEAVE_FLAGS);
	GetIEditor()->GetDisplaySettings()->SetRenderFlags( flags );

	m_roads = FALSE;
	m_decals = FALSE;
	m_detailTex = FALSE;
	m_fog = FALSE;
	m_shadowMaps = FALSE;
	m_skyBox = FALSE;
	m_staticObj = FALSE;
	m_terrain = FALSE;
	m_water = FALSE;
	m_detailObj = FALSE;
	m_particles = FALSE;
	m_clouds = FALSE;
	m_imposters = FALSE;
  if(!gEnv->pSystem->GetI3DEngine()->GetIVoxTerrain())
	  m_voxels = FALSE;
	m_beams = FALSE;
	m_globalIllum = FALSE;
	m_alphablend = FALSE;

	UpdateDisplayOptions();
	SetControls();

}

void CPanelDisplayRender::OnDisplayInvert() 
{
	uint32 flags0 = GetIEditor()->GetDisplaySettings()->GetRenderFlags();
	uint32 flags = ~flags0;
	flags &= ~(LEAVE_FLAGS);
	flags |= flags0 & (LEAVE_FLAGS);
	GetIEditor()->GetDisplaySettings()->SetRenderFlags( flags );

	m_roads =					m_roads				?FALSE:TRUE;
	m_decals =				m_decals			?FALSE:TRUE;
	m_detailTex =			m_detailTex		?FALSE:TRUE;
	m_fog =						m_fog					?FALSE:TRUE;
	m_shadowMaps =		m_shadowMaps	?FALSE:TRUE;
	m_skyBox =				m_skyBox			?FALSE:TRUE;
	m_staticObj =			m_staticObj		?FALSE:TRUE;
	m_terrain =				m_terrain			?FALSE:TRUE;
	m_water =					m_water				?FALSE:TRUE;
	m_detailObj =			m_detailObj		?FALSE:TRUE;
	m_particles =			m_particles		?FALSE:TRUE;
	m_clouds =				m_clouds			?FALSE:TRUE;
	m_imposters =			m_imposters		?FALSE:TRUE;
  if(!gEnv->pSystem->GetI3DEngine()->GetIVoxTerrain())
  	m_voxels =				m_voxels			?FALSE:TRUE;
	m_beams =					m_beams				?FALSE:TRUE;
	m_globalIllum =		m_globalIllum	?FALSE:TRUE;
	m_alphablend =		m_alphablend	?FALSE:TRUE;

	UpdateDisplayOptions();
	SetControls();
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnFillMode() 
{
  if (ICVar * r_wireframe = gEnv->pConsole->GetCVar("r_wireframe"))
    r_wireframe->Set(R_SOLID_MODE);
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnWireframeMode() 
{
  if (ICVar * r_wireframe = gEnv->pConsole->GetCVar("r_wireframe"))
    r_wireframe->Set(R_WIREFRAME_MODE);
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnPointMode()
{
	if (ICVar * r_wireframe = gEnv->pConsole->GetCVar("r_wireframe"))
		r_wireframe->Set(R_POINT_MODE);
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnBnClickedHideHelpers()
{
	if (IsDlgButtonChecked( IDC_HIDE_HELPERS ))
		GetIEditor()->GetDisplaySettings()->DisplayHelpers( false );
	else
		GetIEditor()->GetDisplaySettings()->DisplayHelpers( true );

	GetIEditor()->Notify(eNotify_OnDisplayRenderUpdate);
}

//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnDisplayRenderUpdate:
		SetControls();
		break;
	}
}
//////////////////////////////////////////////////////////////////////////
CPanelDisplayRender*& CPanelDisplayRender::GetCurrentDisplayRender()
{
	static CPanelDisplayRender* poCurrentDisplayRender(NULL);
	return poCurrentDisplayRender;
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDisplayOptionChanged(ICVar* piDisplayModeVariable)
{
	CPanelDisplayRender*												poDisplayMode=CPanelDisplayRender::GetCurrentDisplayRender();
	if (!poDisplayMode)
	{
		return;
	}
	poDisplayMode->OnDisplayOptionChanged();

	TDVariableNameToConsoleFunction::iterator		itIterator;
	itIterator=poDisplayMode->m_cVariableNameToConsoleFunction.find(piDisplayModeVariable->GetName());
	if (itIterator!=poDisplayMode->m_cVariableNameToConsoleFunction.end())
	{
		if (itIterator->second)
		{
			itIterator->second(piDisplayModeVariable);
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDebugOptionChanged(ICVar* piDisplayModeVariable)
{
	CPanelDisplayRender*	poDisplayMode=CPanelDisplayRender::GetCurrentDisplayRender();
	if (!poDisplayMode)
	{
		return;
	}
	poDisplayMode->OnDebugOptionChanged();

	TDVariableNameToConsoleFunction::iterator		itIterator;
	itIterator=poDisplayMode->m_cVariableNameToConsoleFunction.find(piDisplayModeVariable->GetName());
	if (itIterator!=poDisplayMode->m_cVariableNameToConsoleFunction.end())
	{
		if (itIterator->second)
		{
			itIterator->second(piDisplayModeVariable);
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDisplayOptionChanged()
{
	ICVar		*piVariable(NULL);
	int			nDisplayMode(0);

	if (m_boIsUpdatingValues)
	{
		return;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Roads");
	if (piVariable)
	{
		m_roads=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Decals");
	if (piVariable)
	{
		m_decals=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_DetailTextures");
	if (piVariable)
	{
		m_detailTex=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_TerrainDetailMaterials");
	if (piVariable)
	{
		m_detailTex=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}	

	piVariable=gEnv->pConsole->GetCVar("e_Fog");
	if (piVariable)
	{
		m_fog=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Terrain");
	if (piVariable)
	{
		m_terrain=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_SkyBox");
	if (piVariable)
	{
		m_skyBox=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Shadows");
	if (piVariable)
	{
		m_shadowMaps=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Vegetation");
	if (piVariable)
	{
		m_staticObj=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_WaterOcean");
	if (piVariable)
	{
		m_water=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_WaterVolumes");
	if (piVariable)
	{
		m_water=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_ProcVegetation");
	if (piVariable)
	{
		m_detailObj=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Particles");
	if (piVariable)
	{
		m_particles=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Clouds");
	if (piVariable)
	{
		m_clouds=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_Voxel");
	if (piVariable)
	{
		m_voxels=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_VegetationSprites");
	if (piVariable)
	{
		m_imposters=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_Beams");
	if (piVariable)
	{
		m_beams=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("e_GI");
	if (piVariable)
	{
		m_globalIllum=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	UpdateData(FALSE);

	piVariable=gEnv->pConsole->GetCVar("r_wireframe");
	if (piVariable)
	{
		nDisplayMode=piVariable->GetIVal();
	}

	if (nDisplayMode == R_SOLID_MODE)
	{
		CheckDlgButton( IDC_FILL_MODE,BST_CHECKED );
		CheckDlgButton( IDC_WIREFRAME_MODE,BST_UNCHECKED );
		CheckDlgButton( IDC_POINT_MODE,BST_UNCHECKED );
	}
	else if (nDisplayMode == R_WIREFRAME_MODE)
	{
		CheckDlgButton( IDC_FILL_MODE,BST_UNCHECKED);
		CheckDlgButton( IDC_WIREFRAME_MODE,BST_CHECKED);
		CheckDlgButton( IDC_POINT_MODE,BST_UNCHECKED);
	}
	else if (nDisplayMode == R_POINT_MODE)
	{
		CheckDlgButton( IDC_FILL_MODE,BST_UNCHECKED);
		CheckDlgButton( IDC_WIREFRAME_MODE,BST_UNCHECKED);
		CheckDlgButton( IDC_POINT_MODE,BST_CHECKED);
	}
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::UpdateDisplayOptions()
{
	ICVar		*piVariable(NULL);

	m_boIsUpdatingValues=true;

	piVariable=gEnv->pConsole->GetCVar("e_Roads");
	if (piVariable)
	{
		piVariable->Set(m_roads);
	}

	piVariable=gEnv->pConsole->GetCVar("e_Decals");
	if (piVariable)
	{
		piVariable->Set(m_decals);
	}

	piVariable=gEnv->pConsole->GetCVar("r_DetailTextures");
	if (piVariable)
	{
		piVariable->Set(m_detailTex);
	}

	piVariable=gEnv->pConsole->GetCVar("e_TerrainDetailMaterials");
	if (piVariable)
	{
		piVariable->Set(m_detailTex);
	}	

	piVariable=gEnv->pConsole->GetCVar("e_Fog");
	if (piVariable)
	{
		piVariable->Set(m_fog);
	}

	piVariable=gEnv->pConsole->GetCVar("e_Terrain");
	if (piVariable)
	{
		piVariable->Set(m_terrain);
	}

	piVariable=gEnv->pConsole->GetCVar("e_SkyBox");
	if (piVariable)
	{
		piVariable->Set(m_skyBox);
	}

	piVariable=gEnv->pConsole->GetCVar("e_Shadows");
	if (piVariable)
	{
		piVariable->Set(m_shadowMaps);
	}

	piVariable=gEnv->pConsole->GetCVar("e_Vegetation");
	if (piVariable)
	{
		piVariable->Set(m_staticObj);
	}

	piVariable=gEnv->pConsole->GetCVar("e_WaterOcean");
	if (piVariable)
	{
		piVariable->Set(m_water);
	}

	piVariable=gEnv->pConsole->GetCVar("e_WaterVolumes");
	if (piVariable)
	{
		piVariable->Set(m_water);
	}

	piVariable=gEnv->pConsole->GetCVar("e_ProcVegetation");
	if (piVariable)
	{
		piVariable->Set(m_detailObj);
	}

	piVariable=gEnv->pConsole->GetCVar("e_Particles");
	if (piVariable)
	{
		piVariable->Set(m_particles);
	}

	piVariable=gEnv->pConsole->GetCVar("e_Clouds");
	if (piVariable)
	{
		piVariable->Set(m_clouds);
	}

	piVariable=gEnv->pConsole->GetCVar("e_Voxel");
	if (piVariable)
	{
		piVariable->Set(m_voxels);
	}

	piVariable=gEnv->pConsole->GetCVar("e_VegetationSprites");
	if (piVariable)
	{
		piVariable->Set(m_imposters);
	}

	piVariable=gEnv->pConsole->GetCVar("r_Beams");
	if (piVariable)
	{
		piVariable->Set(m_beams);
	}

	piVariable=gEnv->pConsole->GetCVar("e_GI");
	if (piVariable)
	{
		piVariable->Set(m_globalIllum);
	}

	piVariable=gEnv->pConsole->GetCVar("r_wireframe");
	if (piVariable)
	{
		if (IsDlgButtonChecked(IDC_FILL_MODE)==BST_CHECKED)
		{
			piVariable->Set(0);
		}
		else if (IsDlgButtonChecked(IDC_WIREFRAME_MODE)==BST_CHECKED)
		{
			piVariable->Set(1);
		}
		else if (IsDlgButtonChecked(IDC_POINT_MODE)==BST_CHECKED)
		{
			piVariable->Set(2);
		}
	}
	m_boIsUpdatingValues=false;
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::OnDebugOptionChanged()
{
	ICVar		*piVariable(NULL);

	uint32 flags = GetIEditor()->GetDisplaySettings()->GetDebugFlags();
	m_dbg_memory_info = (flags&DBG_MEMINFO)?TRUE:FALSE;

	piVariable=gEnv->pConsole->GetCVar("ai_DebugDraw");
	if (piVariable)
	{
		m_dbg_aidebugdraw=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_TexLog");
	if (piVariable)
	{
		m_dbg_texture_meminfo=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("MemStats");
	if (piVariable)
	{
		m_dbg_mem_stats=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("p_draw_helpers");
	if (piVariable)
	{
		m_dbg_physicsDebugDraw=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}	

	piVariable=gEnv->pConsole->GetCVar("r_ProfileShaders");
	if (piVariable)
	{
		m_dbg_renderer_profile_shaders=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_MeasureOverdraw");
	if (piVariable)
	{
		m_dbg_renderer_overdraw=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_Stats");
	if (piVariable)
	{
		m_dbg_renderer_resources=(piVariable->GetIVal()==1)?TRUE:FALSE;
    m_dbg_renderer_show_shaders=(piVariable->GetIVal()==2)?TRUE:FALSE;
		m_dbg_drawcalls=(piVariable->GetIVal()==6)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("sys_enable_budgetmonitoring");
	if (piVariable)
	{
		m_dbg_budget_monitoring=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("Profile");
	if (piVariable)
	{
		m_dbg_frame_profile=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	piVariable=gEnv->pConsole->GetCVar("r_DebugLights");
	if (piVariable)
	{
		m_dbg_debug_lights=(piVariable->GetIVal()!=0)?TRUE:FALSE;
	}

	UpdateData(FALSE);
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::UpdateDebugOptions()
{
	ICVar		*piVariable(NULL);

	// It is commented here because it's not really necessary as this is not
	// a console variable.
	//uint32 flags = GetIEditor()->GetDisplaySettings()->GetDebugFlags();
	//m_dbg_memory_info = (flags&DBG_MEMINFO)?TRUE:FALSE;
	//GetIEditor()->GetDisplaySettings()->SetDebugFlags(flags);

	piVariable=gEnv->pConsole->GetCVar("ai_DebugDraw");
	if (piVariable)
	{
		piVariable->Set(m_dbg_aidebugdraw);
	}

	piVariable=gEnv->pConsole->GetCVar("ai_DebugDrawCover");
	if (piVariable)
	{
		piVariable->Set(m_dbg_aidebugdrawcover ? 2 : 0);
	}

	piVariable=gEnv->pConsole->GetCVar("r_TexLog");
	if (piVariable)
	{
		piVariable->Set(m_dbg_texture_meminfo?2:0);
	}

	piVariable=gEnv->pConsole->GetCVar("MemStats");
	if (piVariable)
	{
		piVariable->Set(m_dbg_mem_stats? 1000:0 );
	}

	piVariable=gEnv->pConsole->GetCVar("p_draw_helpers");
	if (piVariable)
	{
		piVariable->Set(m_dbg_physicsDebugDraw? 5634:0);
	}	

	piVariable=gEnv->pConsole->GetCVar("r_ProfileShaders");
	if (piVariable)
	{
		piVariable->Set(m_dbg_renderer_profile_shaders);
	}

	piVariable=gEnv->pConsole->GetCVar("r_MeasureOverdraw");
	if (piVariable)
	{
		piVariable->Set(m_dbg_renderer_overdraw? 1:0);
	}

	piVariable=gEnv->pConsole->GetCVar("r_Stats");
	if (piVariable)
	{
    if (m_dbg_renderer_resources)
		  piVariable->Set(1);
    else
    if (m_dbg_renderer_show_shaders)
      piVariable->Set(2);
    else
    if (m_dbg_drawcalls)
      piVariable->Set(6);
    else
      piVariable->Set(0);
	}

	piVariable=gEnv->pConsole->GetCVar("sys_enable_budgetmonitoring");
	if (piVariable)
	{
		piVariable->Set(m_dbg_budget_monitoring? 4:0);
	}

	piVariable=gEnv->pConsole->GetCVar("Profile");
	if (piVariable)
	{
		piVariable->Set(m_dbg_frame_profile? 1:0);
	}

	piVariable=gEnv->pConsole->GetCVar("r_DebugLights");
	if (piVariable)
	{
		piVariable->Set(m_dbg_debug_lights?1:0);
	}
}
//////////////////////////////////////////////////////////////////////////
void CPanelDisplayRender::RegisterChangeCallback(const char* szVariableName,ConsoleVarFunc fnCallbackFunction)
{
	ICVar		*piVariable(NULL);

	piVariable=gEnv->pConsole->GetCVar(szVariableName);
	if (!piVariable)
	{
		return;
	}

	m_cVariableNameToConsoleFunction[(string)szVariableName]=piVariable->GetOnChangeCallback();
	piVariable->SetOnChangeCallback(fnCallbackFunction);
}
//////////////////////////////////////////////////////////////////////////
