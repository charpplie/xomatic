// RenderViewport.cpp : implementation file
//

#include "stdafx.h"
#include "Resource.h"

#include <IGameFramework.h>
#include <IGame.h>

#include "Controls\AnimationToolBar.h"

#include "ModelViewport.h"
#include "CharacterEditor\CharPanel_BAttach.h"
//#include "CharacterEditor\CharPanel_FAttach.h"
#include "CharacterEditor\CharPanel_Animation.h"
#include "CharacterEditor\AnimationBrowser.h"

#include "PropertiesPanel.h"
#include "ThumbnailGenerator.h"
#include "GameEngine.h"


#include "ICryAnimation.h"
#include "CryCharMorphParams.h"
#include "CryCharAnimationParams.h"
#include "IFacialAnimation.h"

#include "FileTypeUtils.h"
#include "Objects\DisplayContext.h"
#include "Material\MaterialManager.h"
#include "ErrorReport.h"

#include <I3DEngine.h>
#include <IPhysics.h>
#include <ITimer.h>
#include "IRenderAuxGeom.h"
#include "DisplaySettings.h"

IMPLEMENT_DYNCREATE(CModelViewport,CRenderViewport)
uint32 g_ypos = 0;

#define SKYBOX_NAME "InfoRedGal"

namespace
{
	int s_varsPanelId = 0;
	CPropertiesPanel* s_varsPanel = 0;
}

#define MULTIPLY (2)

/////////////////////////////////////////////////////////////////////////////
// CModelViewport

CModelViewport::CModelViewport()
{
	m_pCharacterBase = 0;
	m_bPaused = false;

	m_Camera.SetFrustum( 800,600, 3.14f/4.0f, 0.02f,10000 );

	m_pAnimationSystem = GetIEditor()->GetSystem()->GetIAnimationSystem();

	m_bInRotateMode = false;
	m_bInMoveMode = false;

	m_rollupIndex = 0;
	m_rollupIndex2 = 0;

	m_object = 0;
	m_pCompoundCharacter = 0;
	m_pCharacterAnim = 0;
	m_weaponModel = 0;
	m_attachedCharacter = 0;

	m_pAnimationBrowserPanel = 0;
	m_pCharPanel_Animation = 0;
	m_pCharPanel_Preset = 0;
	m_pCharPanel_BAttach = 0;

	m_camRadius = 10;

	m_moveSpeed = 0.1f;
	m_LightRotationRadiant = 0.0f;

	m_weaponIK = false;

	m_pRESky = 0;
	m_pSkyboxName = 0;
	m_pSkyBoxShader = NULL;
	m_pPhysicalEntity = NULL;

	m_attachBone = "weapon_bone";

	// Init variable.
	mv_objectAmbientColor = Vec3(0.40f,0.40f,0.40f);
	mv_backgroundColor		= Vec3(0.20f,0.30f,0.40f);

	m_VPLights.resize(3);

	Vec3 d0 = Vec3(0.70f,0.70f,0.70f);
	mv_lightDiffuseColor0  = d0;
	Vec3 d1 = Vec3(0.40f,0.60f,0.90f);
	mv_lightDiffuseColor1  = d1;
	Vec3 d2 = Vec3(0.40f,0.14f,0.00f);
	mv_lightDiffuseColor2  = d2;


	
	mv_showPhysics = false;




	m_GridOrigin=Vec3(ZERO);
	m_arrAnimatedCharacterPath.resize(0x200);
	m_arrSmoothEntityPath.resize(0x200);

	m_arrRunStrafeSmoothing.resize(0x100);
	SetPlayerPos(Vec3(ZERO));

	//--------------------------------------------------
	// Register variables.
	//--------------------------------------------------
	m_vars.AddVariable( mv_showPhysics,_T("Display Physics"),functor(*this,&CModelViewport::OnShowPhysics) );
	m_vars.AddVariable( mv_noCharPhysics,_T("Disable Char. Phys."),functor(*this,&CModelViewport::OnNoCharPhysics) );
	m_vars.AddVariable( mv_wireframe,_T("Wireframe") );
	m_vars.AddVariable( mv_showGrid,_T("ShowGrid") ); mv_showGrid = true;
	m_vars.AddVariable( mv_showBase,_T("ShowBase") );	mv_showBase = false;
	m_vars.AddVariable( mv_showLocator,_T("ShowLocator") ); mv_showLocator=0;

	m_vars.AddVariable( mv_lighting,_T("Lighting") );	mv_lighting = true;
	m_vars.AddVariable( mv_animateLights,_T("AnimLights") );

	m_vars.AddVariable( mv_backgroundColor,_T("BackgroundColor"),functor(*this,&CModelViewport::OnLightColor),IVariable::DT_COLOR );
	m_vars.AddVariable( mv_objectAmbientColor,_T("ObjectAmbient"),functor(*this,&CModelViewport::OnLightColor),IVariable::DT_COLOR );

	m_vars.AddVariable( mv_lightDiffuseColor0,_T("LightDiffuse1"),functor(*this,&CModelViewport::OnLightColor),IVariable::DT_COLOR );
	m_vars.AddVariable( mv_lightDiffuseColor1,_T("LightDiffuse2"),functor(*this,&CModelViewport::OnLightColor),IVariable::DT_COLOR );
	m_vars.AddVariable( mv_lightDiffuseColor2,_T("LightDiffuse3"),functor(*this,&CModelViewport::OnLightColor),IVariable::DT_COLOR );

	m_vars.AddVariable( mv_showTangents,_T("ShowTangents") );
	m_vars.AddVariable( mv_showBinormals,_T("ShowBinormals") );
	m_vars.AddVariable( mv_showNormals,_T("ShowNormals") );

	m_vars.AddVariable( mv_showSkeleton,_T("ShowSkeleton") );
	m_vars.AddVariable( mv_showJointNames,_T("ShowJointNames") );
	m_vars.AddVariable( mv_showJointsValues,_T("ShowJointsValues") );
	m_vars.AddVariable( mv_showSuperimposed,_T("ShowSuperimposed") );
	m_vars.AddVariable( mv_showRootUpdate,_T("ShowRootUpdate") );
	m_vars.AddVariable( mv_showMotionCaps,_T("ShowMotionCaps") );
	m_vars.AddVariable( mv_showStartLocation,_T("ShowStartLocation") );
	m_vars.AddVariable( mv_showFuturePath,_T("ShowFuturePath") );
	m_vars.AddVariable( mv_showBodyMoveDir,_T("ShowBodyMoveDir") ); mv_showBodyMoveDir=1;
	m_vars.AddVariable( mv_useKeyStrafe,_T("UseKeyStrafe") );
	m_vars.AddVariable( mv_useNaturalSpeed,_T("UseNaturalSpeed") );

	m_vars.AddVariable( mv_printDebugText,_T("PrintDebugText") );
	m_vars.AddVariable( mv_showMotionParam, _T("ShowMotionParam") );
	m_vars.AddVariable( mv_bForceLOD, _T("ForceLOD"), functor(*this,&CModelViewport::OnForceLOD)); mv_bForceLOD = false;
	m_vars.AddVariable( mv_forceLODNum,_T("ForceLODNum"), functor(*this,&CModelViewport::OnForceLODNum));  mv_forceLODNum=0; mv_forceLODNum.SetLimits(0,10);

	
	m_vars.AddVariable( mv_showShaders,_T("ShowShaders"),functor(*this,&CModelViewport::OnShowShaders) );
	m_vars.AddVariable( mv_disableLod,"NoLod",_T("No LOD") );
	m_vars.AddVariable( mv_AttachCamera,_T("AttachCamera") );
	m_vars.AddVariable( mv_CameraSmoothRot,_T("CameraSmoothRot") );  	mv_CameraSmoothRot=0.10f; mv_CameraSmoothRot.SetLimits(0,5);
	m_vars.AddVariable( mv_CameraSmoothPos,_T("CameraSmoothPos") );   mv_CameraSmoothPos=0.20f; mv_CameraSmoothPos.SetLimits(0,5);
	mv_disableLod = true;

	m_vars.AddVariable( mv_fov,_T("FOV") );  mv_fov=60; mv_fov.SetLimits(1,120);
	
	//mv_advancedTable.AddChildVar( mv_ )

	//------------------------------------------------------------------------------
	// This code also tries to restore users' debug option but doesn't work
	//CString xml = AfxGetApp()->GetProfileString( "PreviewSettings","Vars" );
	//if (!xml.IsEmpty())
	//{
	//	XmlParser parser;
	//	XmlNodeRef node = parser.parseBuffer( xml );
	//	if (node)
	//	{
	//		m_vars.Serialize( node,true );
	//	}
	//}
	//------------------------------------------------------------------------------
	// Test
	RestorePreviousUserDebugOptions();

	m_camRadius = 10;

	//YPR_Angle	=	Ang3(0,-1.0f,0);
	//SetViewTM( Matrix34(CCamera::CreateOrientationYPR(YPR_Angle), Vec3(0,-m_camRadius,0))  );
	Vec3 camPos = Vec3(10,10,10);
	Matrix34 tm = Matrix33::CreateRotationVDir( (Vec3(0,0,0) - camPos).GetNormalized() );
	tm.SetTranslation(camPos);
	SetViewTM( tm  );

	if (GetIEditor()->IsInPreviewMode())
	{
		// In preview mode create a simple physical grid, so we can register physical entities.
		int nCellSize = 4;
		IPhysicalWorld *pPhysWorld = gEnv->pPhysicalWorld;
		if (pPhysWorld)
		{
			pPhysWorld->SetupEntityGrid( 2,Vec3(0,0,0),10,10,1,1 );
		}
	}

	m_RT = 0.0f;
	m_LTHUMB	= Vec2(0,0);
	m_RTHUMB	= Vec2(0,0);
	uint32 size = sizeof(m_arrLTHUMB)/sizeof(Vec2);
	for (uint32 i=0; i<size; i++)
		m_arrLTHUMB[i]=Vec2(0,0);

	if (gEnv->pInput)
	{
		gEnv->pInput->AddEventListener( this );
		uint32 test = gEnv->pInput->HasInputDeviceOfType(eIDT_Gamepad);
	}

	// Sound
	m_ListenerID = LISTENERID_INVALID;
}

void CModelViewport::RestorePreviousUserDebugOptions()
{
	CXTRegistryManager regMgr;
	CString str;
	const CString strSection = _T("Settings\\CharacterEditorUserOptions");

	f32 floatValue = .0f;
	UINT byteNum = sizeof(f32);
	CString strRead = "";
	int32 iRead = 0;
	BOOL bRead = FALSE;
	f32 fRead = .0f;
	LPBYTE pbtData = NULL;
	UINT bytes = 0;

	CVarBlock *vb = m_vars.GetVarBlock();
	int32 vbCount = vb->GetVarsCount();

	char keyType[64], keyValue[64];
	for(int32 i=0; i<vbCount; ++i)
	{
		IVariable* var = vb->GetVariable(i);
		IVariable::EType vType = var->GetType();
		sprintf( keyType, "DebugOption_%s_type", var->GetName() );
		int32 iType = regMgr.GetProfileInt(strSection, _T(keyType), 0);
		
		sprintf( keyValue, "DebugOption_%s_value", var->GetName() );
		switch(iType)
		{
		case IVariable::UNKNOWN:
			{
				break;
			}
		case IVariable::INT:
			{
				iRead = regMgr.GetProfileInt(strSection, _T(keyValue), 0);
				var->Set(iRead);
				break;
			}
		case IVariable::BOOL:
			{
				bRead = regMgr.GetProfileInt(strSection, _T(keyValue), FALSE);
				var->Set(bRead);
				break;
			}
		case IVariable::FLOAT:
			{
				regMgr.GetProfileBinary( strSection, _T(keyValue), &pbtData, &bytes);
				fRead = *(f32*)(pbtData);
				var->Set(fRead);
				break;
			}
		case IVariable::VECTOR:
			{
				regMgr.GetProfileBinary( strSection, _T(keyValue), &pbtData, &bytes);
				assert(bytes == 3*sizeof(f32));
				f32* pfRead = (f32*)(pbtData);

				Vec3 vecRead(pfRead[0], pfRead[1], pfRead[2]);
				var->Set(vecRead);
				break;
			}
		case IVariable::QUAT:
			{
				regMgr.GetProfileBinary( strSection, _T(keyValue), &pbtData, &bytes);
				assert(bytes == 4*sizeof(f32));
				f32* pfRead = (f32*)(pbtData);

				Quat valueRead(pfRead[0], pfRead[1], pfRead[2], pfRead[3]);
				var->Set(valueRead);
				break;
			}
		case IVariable::STRING:
			{
				strRead = regMgr.GetProfileString(strSection, _T(keyValue), "");
				var->Set(strRead);
				break;
			}
		case IVariable::ARRAY:
			{
				break;
			}				
		default:
			break;
		}		
	}
}
CModelViewport::~CModelViewport()
{
	ReleaseObject();

	GetIEditor()->FlushUndo();

	//------------------------------------------------------------------------------
	// This code also tries to save users' debug option but cannot be read correctly
	//// Save values to registry.
	//XmlNodeRef node = CreateXmlNode("Vars");
	//m_vars.Serialize( node,false );
	//AfxGetApp()->WriteProfileString( "PreviewSettings","Vars",node->getXML() );

	// Sound
	// cleaning up all sounds, that might still play
	for (int i=0; i<m_SoundIDs.size(); ++i)
	{
		_smart_ptr<ISound> pSound = gEnv->pSoundSystem->GetSound(m_SoundIDs[i]);

		if (pSound)
			pSound->Stop();
	}

	if (m_ListenerID != LISTENERID_INVALID)
		gEnv->pSoundSystem->RemoveListener(m_ListenerID);

	// Remove input event listener
	GetISystem()->GetIInput()->RemoveEventListener( this );

	gEnv->pPhysicalWorld->GetPhysVars()->helperOffset.zero();
	GetIEditor()->SetConsoleVar( "ca_UsePhysics",1 );
}


BEGIN_MESSAGE_MAP(CModelViewport, CRenderViewport)
	ON_WM_CREATE()
	ON_COMMAND(ID_ANIM_BACK, OnAnimBack)
	ON_COMMAND(ID_ANIM_FAST_BACK, OnAnimFastBack)
	ON_COMMAND(ID_ANIM_FAST_FORWARD, OnAnimFastForward)
	ON_COMMAND(ID_ANIM_FRONT, OnAnimFront)
	ON_COMMAND(ID_ANIM_PLAY, OnAnimPlay)
	ON_COMMAND(ID_ANIM_FORCE_PLAY, OnAnimForcePlay)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CModelViewport message handlers
/////////////////////////////////////////////////////////////////////////////
int CModelViewport::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{

	if (CRenderViewport::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::ReleaseObject()
{
	if (m_object)
	{
		m_object->Release();
		m_object = NULL;
	}

	if (GetCharacterBase())
	{
		if (m_pCharPanel_Animation)
		{
			m_pCharPanel_Animation->ClearAnims();
			m_pCharPanel_Animation->SetFileName( "" );
		}
		m_pCompoundCharacter=0;
		m_pCharacterBase=0;
		m_pCharacterAnim=0;
	}

	if (m_pCharacterAnim)
	{
		m_pCharacterAnim=0;
	}

	if (m_weaponModel)
	{
		m_weaponModel->Release();
		m_weaponModel = NULL;
	}
}



//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
bool CModelViewport::OnInputEvent(const SInputEvent &rInputEvent)
{
	uint32 deviceId = rInputEvent.deviceId;
	if (deviceId == eDI_XI)
	{
		uint32 nKeyID = rInputEvent.keyId;
		if (nKeyID==0x210)
			m_LTHUMB.x = rInputEvent.value;
		if (nKeyID==0x211)
			m_LTHUMB.y = rInputEvent.value;

		if (nKeyID==0x216)
			m_RTHUMB.x = rInputEvent.value;
		if (nKeyID==0x217)
			m_RTHUMB.y = rInputEvent.value;

		if (nKeyID==0x20f)
			m_RT = rInputEvent.value;

	//	float color1[4] = {1,1,1,1};
	//	m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"KeyName: %s",rInputEvent.keyName);
	//	g_ypos+=15;
	//	m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"deviceId: %d",deviceId);
	//	g_ypos+=15;
	//	m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"nKeyID: %x         %f",nKeyID,rInputEvent.value); 
	//	g_ypos+=15;
		
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::LoadObject( const CString &fileName,float scale )
{
	m_bPaused = false;

	// Load object.
	CString file = Path::MakeGamePath( fileName );

	bool reload = false;
	if (m_loadedFile == file)
		reload = true;
	m_loadedFile = file;

	SetName( CString("Model View - ") + file );

	ReleaseObject();

	// Do not reload textures.
	//if (m_renderer)
	//m_renderer->FreeResources(FRR_SHADERS|FRR_TEXTURES|FRR_RESTORE);


	// Enables display of warning after model have been loaded.
	CErrorsRecorder errRecorder;


	if (!m_pCharPanel_Animation)	{
		m_pCharPanel_Animation = new CharPanel_Animation(this,this);
		m_rollupIndex = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS,"Character Animation",m_pCharPanel_Animation );
	}
	if (m_pCharPanel_Animation) {
		m_pCharPanel_Animation->SetFileName( file );
	}

	if (!m_pCharPanel_BAttach)	{
		m_pCharPanel_BAttach = new CharPanel_BAttach(this,this);
		m_rollupIndex = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS,"Bone Attachment",m_pCharPanel_BAttach );
		GetIEditor()->ExpandRollUpPage( ROLLUP_OBJECTS,m_rollupIndex,false );
	}

	if (IsPreviewableFileType(file))
	{
		// Try Load character.
		if (stricmp( Path::GetExt(file),CRY_CHARACTER_FILE_EXT ) == 0 ||
			stricmp( Path::GetExt(file),CRY_ANIM_GEOMETRY_FILE_EXT ) == 0 ||
			stricmp( Path::GetExt(file),CRY_CHARACTER_DEFINITION_FILE_EXT ) == 0 ||
			stricmp( Path::GetExt(file),CRY_CHARACTER_PARTS_FILE_EXT ) == 0)
		{








      {







				{
					m_pCharacterBase = m_pAnimationSystem->CreateInstance( file );
				}
      }
			m_pCharacterAnim	=	GetCharacterBase();
			if (GetCharacterBase())
			{
				//m_pCompoundCharacter->AddRef();
				if (m_pCharPanel_Animation && GetCharacterBase() != NULL)
				{
					SetCharacterUIInfo();
				}

				f32  radius = GetCharacterBase()->GetAABB().GetRadius(); //m_pCompoundCharacter->GetRadius();
				Vec3 center = GetCharacterBase()->GetAABB().GetCenter(); //m_pCompoundCharacter->GetCenter();

				m_AABB.min = center-Vec3(radius,radius,radius);
				m_AABB.max = center+Vec3(radius,radius,radius);

				if (!reload)
					m_camRadius = center.z + radius;
			}
		}
		else
		{
			LoadStaticObject( file );
		}
	}
	else
	{
		MessageBox( "Preview of this file type not supported","Preview Error",MB_OK|MB_ICONEXCLAMATION );
		return;
	}

	//--------------------------------------------------------------------------------

	if (!s_varsPanel)
	{
		// Regidet variable pannel.
		s_varsPanel = new CPropertiesPanel(this);
		s_varsPanel->AddVars( m_vars.GetVarBlock() );
		s_varsPanelId = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS,"Debug Options",s_varsPanel );
	}

	//--------------------------------------------------------------------------------


	if (!reload)
	{

		Vec3 v = m_AABB.max - m_AABB.min;
		float radius = v.GetLength()/2.0f;
		m_camRadius = radius*2;
	}

	if (GetIEditor()->IsInPreviewMode())
	{
		Physicalize();
	}

}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::LoadStaticObject(const CString &file )
{
	if (m_object)
		m_object->Release();

	// Load Static object.
	m_object = m_engine->LoadStatObj( file,0,0,false );

	if(!m_object)
	{
		CLogFile::WriteLine( "Loading of object failed." );
		return;
	}
	m_object->AddRef();

	// Generate thumbnail for this cgf.
	CThumbnailGenerator thumbGen;
	thumbGen.GenerateForFile( file );

	m_AABB.min = m_object->GetBoxMin();
	m_AABB.max = m_object->GetBoxMax();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SetCharacterUIInfo()
{
	GetAnimBrowserPanel()->UpdateAnimations(GetCharacterBase());

	// Fill the bone list
	m_pCharPanel_BAttach->ClearBones();

	int numBones = GetCharacterBase()->GetISkeletonPose()->GetJointCount();
	for (int i = 0; i < numBones; i++)
	{
		const char *str = GetCharacterBase()->GetISkeletonPose()->GetJointNameByID(i);
		if (str == NULL)
			break;
		if (strlen(str) > 0)
			m_pCharPanel_BAttach->AddBone(str);
	}
	m_pCharPanel_BAttach->SelectBone( m_attachBone );
}





//////////////////////////////////////////////////////////////////////////
void CModelViewport::UpdateAnimationList()
{
	if (GetCharacterBase()==0)
		return;

	CAnimationBrowser* pAnimBrowserPanel = GetAnimBrowserPanel();
	if(pAnimBrowserPanel)
		pAnimBrowserPanel->UpdateAnimations(GetCharacterBase());
}



static IRenderer *s_pRenderer = 0;
static void g_DrawLine(float *v1,float *v2)
{
	Vec3 V1(v1[0],v1[1],v1[2]);
	Vec3 V2(v2[0],v2[1],v2[2]);
	s_pRenderer->GetIRenderAuxGeom()->DrawLine(V1,ColorB(255,255,255,255),V2,ColorB(255,255,255,255));
}

void CModelViewport::OnRender()
{
	ProcessKeys();
	if (m_renderer)
	{
		m_Camera.SetFrustum( m_Camera.GetViewSurfaceX(),m_Camera.GetViewSurfaceZ(), m_Camera.GetFov(), 0.02f,10000 );
		int w = m_rcClient.right - m_rcClient.left;
		int h = m_rcClient.bottom - m_rcClient.top;	
		m_Camera.SetFrustum( w,h, DEG2RAD(mv_fov), 0.0101f,10000.0f );

		if (GetIEditor()->IsInPreviewMode())
		{
			GetISystem()->SetViewCamera( m_Camera );
		}

		Vec3 clearColor = mv_backgroundColor;
		m_renderer->SetClearColor( clearColor );
		m_renderer->SetCamera( m_Camera );

		m_renderer->ClearBuffer(FRT_CLEAR | FRT_CLEAR_IMMEDIATE, &ColorF(clearColor, 1.0f));
		m_renderer->ResetToDefault();

		IPhysicalWorld *physWorld = GetIEditor()->GetSystem()->GetIPhysicalWorld();
		if (physWorld)
		{
			s_pRenderer = m_renderer;
			//	physWorld->DrawPhysicsHelperInformation(g_DrawLine);
			//	physWorld->DrawPhysicsHelperInformation(m_renderer);
		}

		if (mv_wireframe)
		{
			m_renderer->SetWireframeMode(R_WIREFRAME_MODE);
			DrawModel();
		} else {
			m_renderer->SetWireframeMode(R_SOLID_MODE);
			DrawModel();
		}
	}
	if (!m_object && !m_pCompoundCharacter)
	{
		//LoadObject( "Objects\\box.cgf",1 );
	}
}






//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawSkyBox()
{
	CRenderObject * pObj = m_renderer->EF_GetObject(true, -1);
	pObj->m_II.m_Matrix.SetTranslationMat(GetViewTM().GetTranslation());

	if (m_pSkyboxName)
	{
		m_renderer->EF_AddEf(m_pRESky, SShaderItem(m_pSkyBoxShader), pObj, EFSLIST_GENERAL, 1);
	}
}


void CModelViewport::OnAnimBack() 
{
	// TODO: Add your command handler code here
}

void CModelViewport::OnAnimFastBack() 
{
	// TODO: Add your command handler code here

}

void CModelViewport::OnAnimFastForward() 
{
	// TODO: Add your command handler code here

}

void CModelViewport::OnAnimFront() 
{
	// TODO: Add your command handler code here

}

void CModelViewport::OnAnimPlay() 
{
	// TODO: Add your command handler code here
	if (m_pCharPanel_Animation && GetCharacterBase() != NULL)
	{
		// the name of the currently selected animation
		CString strAnimName = m_pCharPanel_Animation->GetCurrAnimName();
		uint32 looped			= m_pCharPanel_Animation->GetLoopAnimation();
		PlayAnimation(strAnimName);
	}
}

void CModelViewport::OnAnimForcePlay()
{
	if ( m_pCharPanel_Animation && GetCharacterBase() )
	{
		int layer = m_pCharPanel_Animation->GetLayer();
		CryCharAnimationParams Params( layer );

		ISkeletonAnim* pSkeletonAnim = m_pCharacterAnim->GetISkeletonAnim();
		if (pSkeletonAnim)
		{
			int count = pSkeletonAnim->GetNumAnimsInFIFO( layer );
			while ( count-- )
			{
				CAnimation& animation = pSkeletonAnim->GetAnimFromFIFO( layer, count );
				animation.m_AnimParams.m_nFlags &= ~(CA_START_AFTER | CA_START_AT_KEYTIME);
			}
		}
	}
}

void CModelViewport::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default

	CRenderViewport::OnLButtonDblClk(nFlags, point);
	Matrix34 tm;
	tm.SetIdentity();
	SetViewTM(tm);
}

void CModelViewport::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
}

void CModelViewport::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (GetCharacterBase())
	{
		int index = -1;
		if (nChar >= 0x30 && nChar <= 0x39)
		{
			if (nChar == 0x30)
				index = 10;
			else
				index = nChar - 0x31;
		}
		if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9)
		{
			index = nChar - VK_NUMPAD0 + 10;
			if (nChar == VK_NUMPAD0)
				index = 20;
		}
		//if (nFlags& MK_CONTROL

		IAnimationSet* pAnimations = GetCharacterBase()->GetIAnimationSet();
		if (pAnimations)
		{
			uint32 numAnims		= pAnimations->GetAnimationCount();
			uint32 numMorphs	= pAnimations->numMorphTargets();
			if (index >= 0 && index<(numAnims+numMorphs) )
			{
				const char *animName = pAnimations->GetNameByAnimID(index);
				PlayAnimation(animName);
			}
		}
	}
	CRenderViewport::OnKeyDown(nChar, nRepCnt, nFlags);
}







void CModelViewport::OnLightColor( IVariable *var )
{
}



//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowNormals( IVariable *var )
{
	bool enable = mv_showNormals;
	GetIEditor()->SetConsoleVar( "r_ShowNormals",(enable)?1:0 );
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowTangents( IVariable *var )
{
	bool enable = mv_showTangents;
	GetIEditor()->SetConsoleVar( "r_ShowTangents",(enable)?1:0 );
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnForceLOD( IVariable *var)
{
	ICharacterInstance* pICharacterInstance = GetCharacterBase();
	if (pICharacterInstance)
		pICharacterInstance->SetForceLOD(mv_bForceLOD);
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnForceLODNum( IVariable *var)
{
	if(mv_bForceLOD)
	{
		ICharacterInstance* pInstance = GetCharacterBase();
		if(pInstance)
			pInstance->SetForceLODNum(mv_forceLODNum);
	}
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowPhysics( IVariable *var )
{
	bool enable = mv_showPhysics;
	GetIEditor()->SetConsoleVar( "p_draw_helpers_num",(enable)?4354:0 ); // set all bits.
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnNoCharPhysics( IVariable *var )
{
	bool enable = mv_noCharPhysics;
	GetIEditor()->SetConsoleVar( "ca_UsePhysics",(enable)?0:1 );
}



//////////////////////////////////////////////////////////////////////////
void CModelViewport::AttachObjectToBone( const CString &model,const CString &bone )
{
	if (!GetCharacterBase())
		return;

	IAttachmentObject* pBindable = NULL;
	m_attachedCharacter = m_pAnimationSystem->CreateInstance(model);
	if (m_attachedCharacter)
		m_attachedCharacter->AddRef();

	//if (m_attachedCharacter)
	//pBindable = m_attachedCharacter;

	if (m_weaponModel)
	{
		SAFE_RELEASE(m_weaponModel);
	}

	if (!m_attachedCharacter)
	{
		m_weaponModel = m_engine->LoadStatObj (model,0,0,false);
		m_weaponModel->AddRef();
		//if (m_weaponModel)
		//pBindable = m_weaponModel;
	}

	m_attachBone = bone;
	if(!pBindable)
	{
		CString str;
		str.Format( "Loading of weapon model %s failed.",(const char*)model );
		AfxMessageBox( str );
		CLogFile::WriteLine( str );
		return;
	}

	IAttachmentManager* pIAttachments = GetCharacterBase()->GetIAttachmentManager();
	if (m_attachedCharacter) {
		// pIAttachments->AttachToBone ( pBindable,m_pCompoundCharacter->GetModel()->GetBoneByName(bone));
	} else {
		//pIAttachments->CreateAttachmentOld( bone, pBindable, false);
		IAttachment* pAttachment = pIAttachments->CreateAttachment( "BoneAttachment", CA_BONE,bone );
		assert(pAttachment);
		pAttachment->AddBinding(pBindable);
	}
}


//////////////////////////////////////////////////////////////////////////
void CModelViewport::AttachObjectToFace( const CString &model )
{
	if (!GetCharacterBase())
		return;

	if (m_weaponModel)
		m_weaponModel->Release();

	//	m_pCompoundCharacter->ResetAnimations();
	//	m_pCompoundCharacter->InitBones(0);
	//	m_pCompoundCharacter->SetResetMode(1);

	IAttachmentObject* pBindable = NULL;
	m_weaponModel = m_engine->LoadStatObj (model,0,0,false);
	m_weaponModel->AddRef();
	//if (m_weaponModel)
	//pBindable = m_weaponModel;

	if(!pBindable)
	{
		CString str;
		str.Format( "Loading of weapon model %s failed.",(const char*)model );
		AfxMessageBox( str );
		CLogFile::WriteLine( str );
		return;
	}
	IAttachmentManager* pAttachments = GetCharacterBase()->GetIAttachmentManager();
	IAttachment* pIAttachment = pAttachments->CreateAttachment("FaceAttachment",CA_FACE);
	pIAttachment->AddBinding(pBindable);

}


void CModelViewport::StopAnimationInLayer(int nLayer)
{
	if (!GetCharacterBase())
		return;
	GetCharacterBase()->GetISkeletonAnim()->StopAnimationInLayer(nLayer,0.50f);
}


//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnShowShaders( IVariable *var )
{
	bool bEnable = mv_showShaders;
	GetIEditor()->SetConsoleVar("r_ProfileShaders",bEnable);
}

void CModelViewport::OnDestroy()
{
	m_pCharPanel_Animation = 0;

	ReleaseObject();
	if (m_pRESky)
		m_pRESky->Release(false);

	if (m_rollupIndex)
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS,m_rollupIndex );
		m_rollupIndex = 0;
	}
	if (m_rollupIndex2)
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS,m_rollupIndex2 );
		m_rollupIndex2 = 0;
	}
	if (s_varsPanelId)
	{
		GetIEditor()->RemoveRollUpPage( ROLLUP_OBJECTS,s_varsPanelId );
		s_varsPanelId = 0;
	}
	m_pCharPanel_Animation = 0;
	m_pCharPanel_Preset = 0;
	s_varsPanel = 0;

	CRenderViewport::OnDestroy();
}

void CModelViewport::OnSubmeshSetChanged()
{
	SetCharacterUIInfo();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnActivate()
{

}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::OnDeactivate()
{
}

void CModelViewport::Update()
{
	CRenderViewport::Update();

	// Process Sounds
	Vec3 vPos = m_AnimatedCharacter.t;

	for (int i=0; i<m_SoundIDs.size(); )
	{
		_smart_ptr<ISound> pSound = gEnv->pSoundSystem->GetSound(m_SoundIDs[i]);

		if (pSound)
		{
			pSound->SetPosition(vPos);
			++i;
		}
		else
		{
			// remove invalid soundID
			if (i < m_SoundIDs.size()-1)
				m_SoundIDs[i] = m_SoundIDs[m_SoundIDs.size()-1];

			m_SoundIDs.pop_back();
		}
	}

	// Process Listener
	IListener *pListener = NULL;

	if (m_ListenerID == LISTENERID_INVALID)
	{
		// set up a new one if we dont have it yet
		m_ListenerID = gEnv->pSoundSystem->CreateListener();
		pListener = gEnv->pSoundSystem->GetListener(m_ListenerID);
		pListener->SetRecordLevel(1.0f);
		pListener->SetActive(true);
		gEnv->pSoundSystem->Update(eSoundUpdateMode_All);
	}

	// update
	pListener = gEnv->pSoundSystem->GetListener(m_ListenerID);

	if (pListener)
		pListener->SetMatrix(m_Camera.GetMatrix());

	// Update particle effects.
	m_effectManager.Update( m_PhysEntityLocation );
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SetCustomMaterial( CMaterial *pMaterial )
{
	m_pCurrentMaterial = pMaterial;
	//if (m_pCharPanel_Animation)
	//	m_pCharPanel_Animation->SetMaterial(pMaterial);
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CModelViewport::GetMaterial()
{
	if (m_pCurrentMaterial)
		return m_pCurrentMaterial;
	else
	{
		IMaterial *pMtl = 0;
		if (m_object)
			pMtl = m_object->GetMaterial();
		else if (GetCharacterBase())
			pMtl = GetCharacterBase()->GetMaterial();

		CMaterial *pCMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(pMtl);
		return pCMaterial;
	}
}

//////////////////////////////////////////////////////////////////////////
bool CModelViewport::CanDrop( CPoint point,IDataBaseItem *pItem )
{
	if (!pItem)
		return false;

	if (pItem->GetType() == EDB_TYPE_MATERIAL)
	{
		SetCustomMaterial( (CMaterial*)pItem );
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::Drop( CPoint point,IDataBaseItem *pItem )
{
	if (!pItem)
	{
		SetCustomMaterial(NULL);
		return;
	}

	if (pItem->GetType() == EDB_TYPE_MATERIAL)
	{
		SetCustomMaterial( (CMaterial*)pItem );
	}
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::Physicalize()
{
	IPhysicalWorld *pPhysWorld = gEnv->pPhysicalWorld;
	if (!pPhysWorld)
		return;

	if (!m_object && !GetCharacterBase())
		return;

	if (m_pPhysicalEntity)
	{
		pPhysWorld->DestroyPhysicalEntity(m_pPhysicalEntity);
		if( GetCharacterBase() )
		{
			if( GetCharacterBase()->GetISkeletonPose()->GetCharacterPhysics(-1) )
				pPhysWorld->DestroyPhysicalEntity(GetCharacterBase()->GetISkeletonPose()->GetCharacterPhysics(-1));
			GetCharacterBase()->GetISkeletonPose()->SetCharacterPhysics(NULL);
		}
	}


	// Add geometries.
	if (m_object)
	{
		m_pPhysicalEntity = pPhysWorld->CreatePhysicalEntity(PE_STATIC,NULL,NULL,0);
		if(!m_pPhysicalEntity)
			return;
		pe_geomparams params;
		params.flags = geom_colltype_ray;
		for (int i = 0; i < 4; i++)
		{
			if (m_object->GetPhysGeom(i))
				m_pPhysicalEntity->AddGeometry( m_object->GetPhysGeom(i),&params );
		}
		// Add all sub mesh geometries.
		for (int nobj = 0; nobj < m_object->GetSubObjectCount(); nobj++)
		{
			IStatObj *pStatObj = m_object->GetSubObject(nobj)->pStatObj;
			if (pStatObj)
			{
				params.pMtx3x4 = &m_object->GetSubObject(nobj)->tm;
				for (int i = 0; i < 4; i++)
				{
					if (pStatObj->GetPhysGeom(i))
						m_pPhysicalEntity->AddGeometry( pStatObj->GetPhysGeom(i),&params );
				}
			}
		}
	}
	else if (GetCharacterBase())
	{
		GetCharacterBase()->GetISkeletonPose()->DestroyCharacterPhysics();
		m_pPhysicalEntity = pPhysWorld->CreatePhysicalEntity(PE_LIVING);
		IPhysicalEntity *pCharPhys = GetCharacterBase()->GetISkeletonPose()->CreateCharacterPhysics(m_pPhysicalEntity,80.0f,-1,70.0f);
		GetCharacterBase()->GetISkeletonPose()->CreateAuxilaryPhysics( pCharPhys, Matrix34(IDENTITY));
		pe_player_dynamics pd; pd.bActive = 0;
		m_pPhysicalEntity->SetParams(&pd);
		//m_pCompoundCharacter->GetISkeletonPose()->BuildPhysicalEntity(m_pPhysicalEntity,0,-1,1);
	}

	/*Matrix34 tm;
	tm.SetIdentity();
	pe_params_pos par_pos;
	par_pos.pos = Vec3(ZERO);
  par_pos.q = Quat(IDENTITY);
	m_pPhysicalEntity->SetParams(&par_pos);*/

	// Set materials.
	CMaterial *pMaterial = GetMaterial();
	if (pMaterial)
	{
		// Assign custom material to physics.
		int surfaceTypesId[MAX_SUB_MATERIALS];
		memset( surfaceTypesId,0,sizeof(surfaceTypesId) );
		int numIds = pMaterial->GetMatInfo()->FillSurfaceTypeIds(surfaceTypesId);

		pe_params_part ppart;
		ppart.nMats = numIds;
		ppart.pMatMapping = surfaceTypesId;
		(GetCharacterBase() && GetCharacterBase()->GetISkeletonPose()->GetCharacterPhysics() ? 
			GetCharacterBase()->GetISkeletonPose()->GetCharacterPhysics() : m_pPhysicalEntity)->SetParams( &ppart );
	}
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::RePhysicalize()
{
  m_pPhysicalEntity = NULL;
  Physicalize();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::SetPaused(bool bPaused)
{
	if (m_bPaused != bPaused)
	{
		if (bPaused)
		{
			if (GetCharacterBase())
				GetCharacterBase()->SetAnimationSpeed(0.0f);
			if (GetCharacterAnim())
				GetCharacterAnim()->SetAnimationSpeed(0.0f);
		}
		else
		{
			if (GetCharacterBase())
				GetCharacterBase()->SetAnimationSpeed(1.0f);
			if (GetCharacterAnim())
				GetCharacterAnim()->SetAnimationSpeed(1.0f);
		}

		m_bPaused = bPaused;
	}
}





//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
void CModelViewport::DrawModel()
{

	m_vCamPos=GetCamera().GetPosition();	
	//GetISystem()->SetViewCamera( m_Camera );
	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	m_renderer->EF_StartEf();

	//////////////////////////////////////////////////////////////////////////
	// Draw lights.
	//////////////////////////////////////////////////////////////////////////
	if (mv_lighting == true)
	{
		uint32 numLights = m_VPLights.size();
		for (int i=0; i<numLights; i++)
		{
			pAuxGeom->DrawSphere( m_VPLights[i].m_Origin,0.2f,ColorB(255,255,0,255) );
		}
	}



	gEnv->pConsole->GetCVar("ca_DrawTangents")->Set( mv_showTangents );
	gEnv->pConsole->GetCVar("ca_DrawBinormals")->Set( mv_showBinormals );
	gEnv->pConsole->GetCVar("ca_DrawNormals")->Set( mv_showNormals );
	
	
	if(GetIEditor()->Get3DEngine())
	{
		ICVar* pDisplayInfo = gEnv->pConsole->GetCVar("r_DisplayInfo");
		if (pDisplayInfo && pDisplayInfo->GetIVal()!=0)
		{
			float fps = gEnv->pTimer->GetFrameRate();

			char text[512];
			sprintf(text, "ResetMode");

			//take overscan borders into account
			float x = (float)gEnv->pRenderer->GetWidth();
			Vec2 overscanBorders = *(Vec2*)gEnv->pRenderer->EF_Query(EFQ_OverscanBorders);
			x -= ((x * overscanBorders.x) + 5.0f);
			float yDelta = ((float)gEnv->pRenderer->GetHeight()) * overscanBorders.y;

			gEnv->p3DEngine->DrawTextRightAligned( x, 1+yDelta, "FPS: %.2f",fps );

			int nPolygons, nShadowVolPolys;
			gEnv->pRenderer->GetPolyCount(nPolygons,nShadowVolPolys);
			int nDrawCalls = gEnv->pRenderer->GetCurrentNumberOfDrawCalls();
			gEnv->p3DEngine->DrawTextRightAligned( x, 20+yDelta, "Tris:%2d,%03d  DP:%d",nPolygons/1000, nPolygons%1000, nDrawCalls );
		}
	}


	if (mv_animateLights)
		m_LightRotationRadiant += m_AverageFrameTime;


	Matrix33 LightRot33	=	Matrix33::CreateRotationZ(m_LightRotationRadiant);

	uint32 numLights = m_VPLights.size();

	float orbit = 15;

	Vec3 LPos0 = Vec3(-orbit,orbit,orbit); 

	m_VPLights[0].m_Origin = LightRot33*LPos0;

	Vec3 d0 = mv_lightDiffuseColor0;
	m_VPLights[0].SetLightColor(ColorF(d0.x*MULTIPLY,d0.y*MULTIPLY,d0.z*MULTIPLY,0));

	m_VPLights[0].m_SpecMult = 1.0f;

	m_VPLights[0].m_fRadius = 400;
	//m_VPLights[0].m_fStartRadius = 5;
	//m_VPLights[0].m_fEndRadius = 400;
	m_VPLights[0].m_Flags |= DLF_POINT;

	//-----------------------------------------------

	Vec3 LPos1 = Vec3(-orbit,-orbit,-orbit/2); 
	m_VPLights[1].m_Origin = LightRot33*LPos1;

	Vec3 d1 = mv_lightDiffuseColor1;
	m_VPLights[1].SetLightColor(ColorF(d1.x*MULTIPLY,d1.y*MULTIPLY,d1.z*MULTIPLY,0));

	m_VPLights[1].m_SpecMult = 1.0f;

	m_VPLights[1].m_fRadius = 400;
	//m_VPLights[1].m_fStartRadius = 5;
	//m_VPLights[1].m_fEndRadius = 400;
	m_VPLights[1].m_Flags |= DLF_POINT;

	//---------------------------------------------

	Vec3 LPos2 = Vec3(orbit,-orbit,0); 
	m_VPLights[2].m_Origin = LightRot33*LPos2;

	Vec3 d2 = mv_lightDiffuseColor2;
	m_VPLights[2].SetLightColor(ColorF(d2.x*MULTIPLY,d2.y*MULTIPLY,d2.z*MULTIPLY,0));

	m_VPLights[2].m_SpecMult = 1.0f;

	m_VPLights[2].m_fRadius = 400;
	//m_VPLights[2].m_fStartRadius = 5;
	//m_VPLights[2].m_fEndRadius = 400;
	m_VPLights[2].m_Flags |= DLF_POINT;


	if (mv_lighting == true)
	{
		// Add lights.
		uint32 numLights = m_VPLights.size();
		if (numLights==1)
		{
			m_renderer->EF_ADDDlight( &m_VPLights[0] );
		}

		if (numLights==2)
		{
			m_renderer->EF_ADDDlight( &m_VPLights[0] );
			m_renderer->EF_ADDDlight( &m_VPLights[1] );
		}

		if (numLights==3)
		{
			m_renderer->EF_ADDDlight( &m_VPLights[0] );
			m_renderer->EF_ADDDlight( &m_VPLights[1] );
			m_renderer->EF_ADDDlight( &m_VPLights[2] );
		}

	}

	//-------------------------------------------------------------
	//------           Render physical Proxy                 ------
	//-------------------------------------------------------------
	m_renderer->Set2DMode( true,m_rcClient.right,m_rcClient.bottom );
	if (m_pPhysicalEntity)
	{
		CPoint mousePoint;
		GetCursorPos(&mousePoint);
		ScreenToClient(&mousePoint);
		Vec3 raySrc,rayDir;
		ViewToWorldRay( mousePoint,raySrc,rayDir );

		ray_hit hit;
		hit.pCollider = 0;
		int flags = rwi_any_hit|rwi_stop_at_pierceable;
		int col = 0;
		if ( raySrc.IsValid() && rayDir.IsValid() )
		{
			col = gEnv->pPhysicalWorld->RayWorldIntersection( raySrc,rayDir*1000.0f,ent_static,flags,&hit,1 );
		}
		if (col > 0)
		{
			int nMatId = hit.idmatOrg;

			string sMaterial;
			if (GetMaterial())
			{
				sMaterial = GetMaterial()->GetMatInfo()->GetSafeSubMtl(nMatId)->GetName();
			}

			ISurfaceType *pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType( hit.surface_idx,m_loadedFile );
			if (pSurfaceType)
			{
				float color[4] = {1,1,1,1};
				//m_renderer->Draw2dLabel( mousePoint.x+12,mousePoint.y+8,1.2f,color,false,"%s\n%s",sMaterial.c_str(),pSurfaceType->GetName() );
			}
		}
	}
	m_renderer->Set2DMode( false,m_rcClient.right,m_rcClient.bottom );



	//-----------------------------------------------------------------------------
	//-----            Render Static Object (handled by 3DEngine)              ----
	//-----------------------------------------------------------------------------
	// calculate LOD
//	IConsole* pIConsole = GetIEditor()->GetSystem()->GetIConsole();
//	ICVar* pICVar = pIConsole->GetCVar("e_LodRatio");
//	if (pICVar)
//		pICVar->Set( 6.0f );

	f32 fDistance = GetViewTM().GetTranslation().GetLength();
/*	if (mv_disableLod)
	{
		fDistance = 0;
		if (pICVar)
			pICVar->Set( 999999.0f );
	}*/
	SRendParams rp;
	rp.fDistance	= fDistance;

	Matrix34 tm; tm.SetIdentity();
	rp.pMatrix = &tm;
	rp.pPrevMatrix = &tm;

	Vec3 vAmbient;
	mv_objectAmbientColor.Get(vAmbient);

//	float color1[4] = {1,1,1,1};
//	g_ypos+=100;
//	m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"vObjectAmbient: %f %f %f",vAmbient.x,vAmbient.y,vAmbient.z );	
//	g_ypos+=10;

	rp.AmbientColor.r=vAmbient.x*MULTIPLY;
	rp.AmbientColor.g=vAmbient.y*MULTIPLY;
	rp.AmbientColor.b=vAmbient.z*MULTIPLY;
	rp.AmbientColor.a = 1;

	rp.nDLightMask = 7;
	if (mv_lighting == true)rp.nDLightMask = 0xffff;
	else rp.nDLightMask = 0;
	rp.nDLightMask;

	rp.dwFObjFlags  = 0;
	rp.dwFObjFlags |= FOB_TRANS_MASK;
	if (m_pCurrentMaterial)
		rp.pMaterial = m_pCurrentMaterial->GetMatInfo();

	// Render particle effects.
	m_effectManager.Render(rp);

	//-----------------------------------------------------------------------------
	//-----            Render Static Object (handled by 3DEngine)              ----
	//-----------------------------------------------------------------------------
	if (m_object)
	{
		m_object->Render( rp );
		if (mv_showGrid)
			DrawGrid( Quat(IDENTITY), Vec3(ZERO), Vec3(ZERO), Matrix33(IDENTITY));

		if (mv_showBase)
			DrawCoordSystem( IDENTITY,10.0f); 		//DrawCoordSystem( QuatT(m_AnimatedCharacter) ,10.0f);
	}





	//-----------------------------------------------------------------------------
	//-----             Render Character (handled by CryAnimation)            ----
	//-----------------------------------------------------------------------------
	if (GetCharacterBase())
	{
		DrawCharacter( GetCharacterBase(), rp );

		if (GetCharacterBase() && !(GetIEditor()->GetDisplaySettings()->GetSettings() & SETTINGS_PHYSICS) && m_AverageFrameTime>0 && !mv_noCharPhysics)	
		{
			IPhysicalEntity *pCharPhys;
			int stepents = ent_independent;
			pe_params_flags pf; pf.flagsOR = pef_update;
			for(int i=-1; pCharPhys=GetCharacterBase()->GetISkeletonPose()->GetCharacterPhysics(i); i++)
				pCharPhys->SetParams(&pf);

			pe_action_move am;
			am.dir = GetCharacterBase()->GetISkeletonAnim()->GetCurrentVelocity(); am.iJump = 1;
			m_pPhysicalEntity->Action(&am);
			if ( UseAnimationDrivenMotion() ) {
				gEnv->pPhysicalWorld->GetPhysVars()->helperOffset -= am.dir = GetCharacterBase()->GetISkeletonAnim()->GetRelMovement().t;
				gEnv->pPhysicalWorld->GetPhysVars()->timeScalePlayers = 1.0f;
				am.dir /= m_AverageFrameTime;
				m_pPhysicalEntity->SetParams(&pf); stepents |= ent_living;
			} 
			else
				gEnv->pPhysicalWorld->GetPhysVars()->helperOffset = am.dir.zero();
			am.iJump = 3;
			m_pPhysicalEntity->Action(&am);

			gEnv->pPhysicalWorld->TimeStep(m_AverageFrameTime, stepents|ent_flagged_only);	
		}
			//m_pCompoundCharacter->GetISkeletonPose()->SynchronizeWithPhysicalEntity(m_pPhysicalEntity);

		m_EntityMat = Matrix34(m_PhysEntityLocation);

	//	float color1[4] = {1,1,1,1};
	//	g_ypos+=100;
	//	m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"m_PhysEntityLocation: %f %f %f",m_PhysEntityLocation.t.x,m_PhysEntityLocation.t.y,m_PhysEntityLocation.t.z );	
	//	g_ypos+=10;
		

		pe_params_pos par_pos;
		par_pos.pMtx3x4 = &m_EntityMat;
		if (!GetCharacterBase() || !UseAnimationDrivenMotion())
		m_pPhysicalEntity->SetParams(&par_pos);
	}

	m_renderer->EF_EndEf3D(SHDF_SORT/* | SHDF_ZPASS*/ | SHDF_STREAM_SYNC, -1);
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::PlayAnimation( const char*szName )
{

}


//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawGrid( const Quat& m33, const Vec3& MotionTranslation,const Vec3& FootSlide, const Matrix33& rGridRot)
{
	if (!m_renderer)
		return;


	
	bool bTransRot = UseAnimationDrivenMotion();
	if (bTransRot)
		m_GridOrigin += m33*MotionTranslation + FootSlide;


	if (m_GridOrigin.x>+1.0f) m_GridOrigin.x-=1.0f; 
	if (m_GridOrigin.y>+1.0f) m_GridOrigin.y-=1.0f; 
	//	if (m_GridOrigin.z>+1.0f) m_GridOrigin.z-=1.0f; 

	if (m_GridOrigin.x<-1.0f) m_GridOrigin.x+=1.0f; 
	if (m_GridOrigin.y<-1.0f) m_GridOrigin.y+=1.0f; 
	//	if (m_GridOrigin.z<-1.0f) m_GridOrigin.z+=1.0f; 

	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );

	float XR = 45;
	float YR = 45;
	Vec3 axis=m33.GetColumn0();

	Matrix33 SlopeMat33=rGridRot;
	// [MichaelS 13/2/2007] Stop breaking the facial editor! Don't assume m_pCharPanel_Animation is valid! You (and I) know who you are!
	uint32 GroundAlign = (m_pCharPanel_Animation ? m_pCharPanel_Animation->GetGroundAlign() : 1);
	if (GroundAlign==0)
		SlopeMat33= Matrix33::CreateRotationAA( -m_absCurrentSlope, axis);

	// Draw grid.
	float step = 0.25f;
	for (float x = -XR; x < XR; x+=step)
	{
		Vec3 p0=Vec3(x,-YR,0)+m_GridOrigin;		Vec3 p1=Vec3(x,YR,0)+m_GridOrigin;
		pAuxGeom->DrawLine( SlopeMat33*p0,RGBA8(0x7f,0x7f,0x7f,0x00), SlopeMat33*p1,RGBA8(0x7f,0x7f,0x7f,0x00) );
	}
	for (float y = -YR; y < YR; y+=step)
	{
		Vec3 p0=Vec3(-XR,y,0)+m_GridOrigin;		Vec3 p1=Vec3(XR,y,0)+m_GridOrigin;
		pAuxGeom->DrawLine( SlopeMat33*p0,RGBA8(0x7f,0x7f,0x7f,0x00), SlopeMat33*p1,RGBA8(0x7f,0x7f,0x7f,0x00) );
	}
}


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
void CModelViewport::DrawCoordSystem( const QuatT& location, f32 length )
{
	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	pAuxGeom->SetRenderFlags( renderFlags );

	Vec3 absAxisX	=	location.q.GetColumn0();
	Vec3 absAxisY	=	location.q.GetColumn1();
	Vec3 absAxisZ	= location.q.GetColumn2();

	const f32 scale=1.0f;
	const f32 size=0.009f;
	AABB xaabb = AABB(Vec3(-length*scale, -size*scale, -size*scale),Vec3(length*scale, size*scale,	 size*scale));
	AABB yaabb = AABB(Vec3(-size*scale, -length*scale, -size*scale),Vec3(size*scale,	 length*scale, size*scale));
	AABB zaabb = AABB(Vec3(-size*scale, -size*scale, -length*scale),Vec3(size*scale,	 size*scale,	 length*scale));

	OBB obb;
	obb =	OBB::CreateOBBfromAABB( Matrix33(location.q), xaabb );
	pAuxGeom->DrawOBB(obb,location.t,1,RGBA8(0xff,0x00,0x00,0xff),eBBD_Extremes_Color_Encoded);
	pAuxGeom->DrawCone(location.t+absAxisX*length*scale,absAxisX,0.03f,0.15f,RGBA8(0xff,0x00,0x00,0xff));

	obb =	OBB::CreateOBBfromAABB( Matrix33(location.q), yaabb );
	pAuxGeom->DrawOBB(obb,location.t,1,RGBA8(0x00,0xff,0x00,0xff),eBBD_Extremes_Color_Encoded);
	pAuxGeom->DrawCone(location.t+absAxisY*length*scale,absAxisY,0.03f,0.15f,RGBA8(0x00,0xff,0x00,0xff));

	obb =	OBB::CreateOBBfromAABB( Matrix33(location.q), zaabb );
	pAuxGeom->DrawOBB(obb,location.t,1,RGBA8(0x00,0x00,0xff,0xff),eBBD_Extremes_Color_Encoded);
	pAuxGeom->DrawCone(location.t+absAxisZ*length*scale,absAxisZ,0.03f,0.15f,RGBA8(0x00,0x00,0xff,0xff));
}

//////////////////////////////////////////////////////////////////////////
bool CModelViewport::UseAnimationDrivenMotion() const
{
	return ( m_pCharPanel_Animation ? m_pCharPanel_Animation->GetAnimationDrivenMotion() : false );
}

//////////////////////////////////////////////////////////////////////////
void CModelViewport::DrawCharacter( ICharacterInstance* pInstance, const SRendParams &rRP )
{
	g_ypos = 162;
	float color1[4] = {1,1,1,1};

	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	pAuxGeom->SetRenderFlags( renderFlags );

	f32 FrameTime = GetIEditor()->GetSystem()->GetITimer()->GetFrameTime();
	m_AverageFrameTime = pInstance->GetAverageFrameTime(); 



	ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();
	bool bTransRot2000	= UseAnimationDrivenMotion();
	pISkeletonAnim->SetAnimationDrivenMotion(bTransRot2000);

	uint32 rm = pInstance->GetResetMode();
	if (rm && gEnv->p3DEngine) 
	{
		char text[512];
		sprintf(text, "ResetMode");

		//take overscan borders into account
		float x = (float)gEnv->pRenderer->GetWidth();
		Vec2 overscanBorders = *(Vec2*)gEnv->pRenderer->EF_Query(EFQ_OverscanBorders);
		x -= ((x * overscanBorders.x) + 5.0f);
		float yDelta = ((float)gEnv->pRenderer->GetHeight()) * overscanBorders.y;

		gEnv->p3DEngine->DrawTextRightAligned( x, 100+yDelta, text );
	}


	f32 fZoomFactor = 0.01f+0.99f*(RAD2DEG(GetCamera().GetFov())/90.f);  
	f32 fDistance = GetViewTM().GetTranslation().GetLength()*fZoomFactor;
	if (mv_disableLod)
		fDistance = 0;

	m_EntityMat=Matrix34(m_PhysEntityLocation);
	m_PrevEntityMat=m_EntityMat;
}

CAnimatedCharacterEffectManager::CAnimatedCharacterEffectManager()
:	m_pSkeleton2(0),m_pSkeletonPose(0)
{
}

CAnimatedCharacterEffectManager::~CAnimatedCharacterEffectManager()
{
	KillAllEffects();
}

void CAnimatedCharacterEffectManager::SetSkeleton(ISkeletonAnim* pSkeletonAnim,ISkeletonPose* pSkeletonPose)
{
	m_pSkeleton2 = pSkeletonAnim;
	m_pSkeletonPose = pSkeletonPose;
}

void CAnimatedCharacterEffectManager::Update(const QuatT& rPhysEntity )
{
	for (int i = 0; i < m_effects.size();)
	{
		EffectEntry& entry = m_effects[i];

		// If the animation has stopped, kill the effect.
		bool effectStillPlaying = (entry.pEmitter ? entry.pEmitter->IsAlive() : false);
		bool animStillPlaying = IsPlayingAnimation(entry.animID);
		if (animStillPlaying && effectStillPlaying)
		{
			// Update the effect position.
			Matrix34 tm;
			GetEffectTM(tm, entry.boneID, entry.offset, entry.dir);
			if (entry.pEmitter)
				entry.pEmitter->SetMatrix(Matrix34(rPhysEntity)*tm);
			 ++i;
		}
		else
		{
#if defined(USER_michaels)
			CryLogAlways("CAnimatedCharacterEffectManager::Update(): Killing effect \"%s\" because %s.",
				(m_effects[i].pEffect ? m_effects[i].pEffect->GetName() : "<EFFECT NULL>"), (effectStillPlaying ? "animation has ended" : "effect has ended"));
#endif //defined(USER_michaels)
			if (m_effects[i].pEmitter)
				m_effects[i].pEmitter->Activate(false);
			m_effects.erase(m_effects.begin() + i);
		}
	}
}

void CAnimatedCharacterEffectManager::KillAllEffects()
{
	for (int i = 0, count = m_effects.size(); i < count; ++i)
	{
		if (m_effects[i].pEmitter)
			m_effects[i].pEmitter->Activate(false);
	}
	m_effects.clear();
}

void CAnimatedCharacterEffectManager::SpawnEffect(int animID, const char* animName, const char* effectName, const char* boneName, const Vec3& offset, const Vec3& dir)
{
	// Check whether we are already playing this effect, and if so dont restart it.
	bool alreadyPlayingEffect = IsPlayingEffect(effectName);

	if (alreadyPlayingEffect)
	{
#if defined(USER_michaels)
		CryLogAlways("CAnimatedCharacterEffectManager::SpawnEffect(): Refusing to start effect \"%s\" because effect is already running.", (effectName ? effectName : "<MISSING EFFECT NAME>"));
#endif //defined(USER_michaels)
	}
	else
	{
		IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(effectName);
		if (!pEffect)
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Anim events cannot find effect \"%s\", requested by animation \"%s\".", (effectName ? effectName : "<MISSING EFFECT NAME>"), (animName ? animName : "<MISSING ANIM NAME>"));
		int boneID = (boneName && boneName[0] && m_pSkeletonPose ? m_pSkeletonPose->GetJointIDByName(boneName) : -1);
		boneID = (boneID == -1 ? 0 : boneID);
		Matrix34 tm;
		GetEffectTM(tm, boneID, offset, dir);
		IParticleEmitter* pEmitter = (pEffect ? pEffect->Spawn(false, tm) : 0);

		// Make sure the emitter is not rendered in the game.
		SpawnParams sp;
		sp.bIgnoreLocation = true;
		pEmitter->SetSpawnParams(sp);

		if (pEffect && pEmitter)
		{
#if defined(USER_michaels)
			CryLogAlways("CAnimatedCharacterEffectManager::SpawnEffect(): starting effect \"%s\", requested by animation \"%s\".", (effectName ? effectName : "<MISSING EFFECT NAME>"), (animName ? animName : "<MISSING ANIM NAME>"));
#endif defined(USER_michaels)
			m_effects.push_back(EffectEntry(pEffect, pEmitter, boneID, offset, dir, animID));
		}
	}
}

CAnimatedCharacterEffectManager::EffectEntry::EffectEntry(_smart_ptr<IParticleEffect> pEffect, _smart_ptr<IParticleEmitter> pEmitter, int boneID, const Vec3& offset, const Vec3& dir, int animID)
:	pEffect(pEffect), pEmitter(pEmitter), boneID(boneID), offset(offset), dir(dir), animID(animID)
{
}

CAnimatedCharacterEffectManager::EffectEntry::~EffectEntry()
{
}

void CAnimatedCharacterEffectManager::GetEffectTM(Matrix34& tm, int boneID, const Vec3& offset, const Vec3& dir)
{
	if (dir.len2()>0)
		tm = Matrix33::CreateRotationXYZ(Ang3(dir * 3.14159f / 180.0f));
	else
		tm.SetIdentity();
	tm.AddTranslation(offset);

	if (m_pSkeletonPose)
		tm = Matrix34(m_pSkeletonPose->GetAbsJointByID(boneID)) * tm;
}

bool CAnimatedCharacterEffectManager::IsPlayingAnimation(int animID)
{
	enum {NUM_LAYERS = 4};

	// Check whether the animation has stopped.
	bool animPlaying = false;
	for (int layer = 0; layer < NUM_LAYERS; ++layer)
	{
		for (int animIndex = 0, animCount = (m_pSkeleton2 ? m_pSkeleton2->GetNumAnimsInFIFO(layer) : 0); animIndex < animCount; ++animIndex)
		{
			CAnimation& anim = m_pSkeleton2->GetAnimFromFIFO(layer, animIndex);
			int32 id = (anim.m_Parametric.m_nParametricID >= 0) ? anim.m_Parametric.m_nParametricID : anim.m_Parametric.m_nAnimID[0];
			animPlaying = animPlaying || (id == animID);
		}
	}

	return animPlaying;
}

bool CAnimatedCharacterEffectManager::IsPlayingEffect(const char* effectName)
{
	bool isPlaying = false;
	for (int effectIndex = 0, effectCount = m_effects.size(); effectIndex < effectCount; ++effectIndex)
	{
		IParticleEffect* pEffect = m_effects[effectIndex].pEffect;
		if (pEffect && stricmp(pEffect->GetName(), effectName) == 0)
			isPlaying = true;
	}
	return isPlaying;
}

void CAnimatedCharacterEffectManager::Render(SRendParams& params)
{
	for (int effectIndex = 0, effectCount = m_effects.size(); effectIndex < effectCount; ++effectIndex)
	{
		IParticleEmitter* pEmitter = m_effects[effectIndex].pEmitter;
		pEmitter->Update();
		pEmitter->Render(params);
	}
}
