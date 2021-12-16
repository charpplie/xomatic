// CharacterEditor.cpp : implementation file
//

#include "stdafx.h"

#include <IRenderer.h>
#include <I3DEngine.h>
#include <ICryAnimation.h>
#include <ITimer.h>
#include <IGame.h>
#include <IGameFramework.h>
#include <IRenderAuxGeom.h>
#include <IFacialAnimation.h>

#include "CryEdit.h"
#include "PropertiesPanel.h"
#include "ThumbnailGenerator.h"
#include "FileTypeUtils.h"
#include "ToolbarDialog.h"

#include "CharPanel_Animation.h"
#include "CharPanel_Attachments.h"
#include "CharPanel_Morphing.h"
#include "ModelViewport.h"
#include "ModelViewportCE.h"
#include "CryCharMorphParams.h"
#include "CharPanel_AnimationControl.h"
#include "CharPanel_Character.h"
#include "CharacterEditor.h"

#include "ICryAnimation.h"
#include "CryCharMorphParams.h"
#include "CryCharAnimationParams.h"

#include "Util\UIEnumerations.h"



#include "Material\MaterialManager.h"


IMPLEMENT_DYNCREATE(CModelViewportCE,CModelViewport)

BEGIN_MESSAGE_MAP(CModelViewportCE, CModelViewport)
	ON_COMMAND(ID_ANIM_PLAY, OnAnimPlay)
	ON_COMMAND(ID_ANIM_FORCE_PLAY, OnAnimForcePlay)

	ON_WM_MOUSEMOVE()

	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()

END_MESSAGE_MAP()

CModelViewportCE::CModelViewportCE()
{
	m_pDefaultMaterial = 0;
	InitDisplayContext();
	m_pAttachmentsDlg=0;
	m_pMorphingDlg=0;
	m_pAnimationControlDlg = 0;
	m_pCharacterPropertiesDlg = 0;
	m_pCharacterChangeListener = 0;
	m_pCharacterEditor = 0;
	m_pCharacterPartsDlg = 0;
	m_pMotionAdaptorDlg = 0;
	
	m_bRenderStats = false;

	InitModelViewportCE();
}

//-------------------------------------------------------------------------
CModelViewportCE::~CModelViewportCE()
{
}

void CModelViewportCE::InitModelViewportCE()
{
	m_ArcBall.InitArcBall();

	//m_arrVertices.resize(0x4000);
	//m_arrIndices.resize(m_arrVertices.size()*3);

	m_WinRect.left	=0;
	m_WinRect.top		=0;
	m_WinRect.right	=0;
	m_WinRect.bottom=0;

	m_Button_MOVE		= 0;
	m_Button_ROTATE	= 0;
	m_MouseButtonL		= false;
	m_MouseButtonR		= false;
	m_SelectionUpdate	= 0;
	m_MouseOnAttachment=-1;
	m_SelectedAttachment=0;

	//init keyboard
	memset(m_bKey, 0x00, sizeof(m_bKey));
	memset(m_keyrcr, 0x00, sizeof(m_keyrcr));
	memset(m_keyflip, 0x00, sizeof(m_keyflip));
}



//////////////////////////////////////////////////////////////////////////
void CModelViewportCE::SetCharacter( ICharacterInstance *pInstance )
{
	if (!pInstance)
		return;










	{
		m_pCharacterBase = pInstance;
		m_pCharacterAnim = m_pCharacterBase;
		
	}

	
	m_AABB = pInstance->GetAABB();

	m_camRadius = (m_AABB.max-m_AABB.min).GetLength();
	m_absCameraPosVP = Vec3( 0, -m_camRadius, (m_AABB.max.z+m_AABB.min.z)/2 );
	SetViewTM( Matrix34::CreateTranslationMat( m_absCameraPosVP ) );
	

	if (m_pCharacterPropertiesDlg)
		m_pCharacterPropertiesDlg->SetMaterial( GetMaterial() );

	Physicalize();

	SendOnCharacterChanged();
}

//#define NUM_INSTANCE 2000
//ICharacterInstance* arrCharacterBase[NUM_INSTANCE];
//////////////////////////////////////////////////////////////////////////
void CModelViewportCE::LoadObject( const CString &fileName, float )
{
	CErrorsRecorder errorRecorder;

	m_bPaused = false;

	// Load object.
	CString file = fileName;

	bool reload = false;
	if (m_loadedFile == file)
		reload = true;
	m_loadedFile = file;

  m_pPhysicalEntity = NULL;

	SetName( CString("Model View - ") + file );
	ReleaseObject();

	// Initialize these with some values
	m_AABB.min = Vec3(-1.0f,-1.0f,-1.0f);
	m_AABB.max = Vec3(1.0f,1.0f,1.0f);

	//assert(m_pCharPanel_Animation);
	if (m_pCharPanel_Animation) 
	{
		m_pCharPanel_Animation->SetFileName( fileName );
	}

	if (IsPreviewableFileType(file))
	{
		m_pCompoundCharacter=0;
		m_pCharacterBase=0;
		m_pCharacterAnim=0;
		string fileExt = Path::GetExt(file);

		bool IsCGA = (0 == stricmp(fileExt,"cga"));
		if (IsCGA)
		{ 
			CLogFile::WriteLine("Loading CGA Model...");










			{
				m_pCharacterBase = m_pAnimationSystem->CreateInstance( file );
				m_pCharacterAnim = m_pCharacterBase;
			}
     
		}

		bool IsCDF = (0 == stricmp(fileExt,"cdf"));
		if (IsCDF) 
		{
			CLogFile::WriteLine("Importing Character Definitions...");










			{
				m_pCharacterBase = m_pAnimationSystem->CreateInstance( file );
				m_pCharacterAnim = m_pCharacterBase;
			}



		/*	for (uint32 i=0; i<NUM_INSTANCE; i++)
				arrCharacterBase[i] = m_pAnimationSystem->CreateInstance( file );*/
		


/*
			IAttachmentManager* pIAttachmentManager = m_pCompoundCharacter->GetIAttachmentManager();
			//STEP 1: create the socket for the face-attachment
			IAttachment* pIAttachment = pIAttachmentManager->CreateAttachment("C4_KaWummmmm",CA_FACE,0);
			//STEP 2: place the socket at the right position on the character
			if (pIAttachment) 
			{
				uint32 type = pIAttachment->GetType();
				if (type==CA_BONE || type==CA_FACE) 
				{
					//				<Attachment AName="back01" Type="CA_FACE" Rotation="-0.075126655,-0.99558783,0.03969615,   0.03981521" Position="0.041536111,-0.15446994,1.4762511" BoneName="" Binding="objects/characters/attachment/nanosuit/nanosuit_pouch.cgf" Flags="0" Material="objects/characters/attachment/nanosuit/Nanosuit_pockets"/>
					Quat WRot=Quat(-0.075126655f,-0.99558783f,0.03969615f,0.03981521f);
					Vec3 WPos=Vec3(0.041536111f,-0.15446994f,1.4762511f);
					pIAttachment->SetRMWRotation( WRot );
					pIAttachment->SetRMWPosition( WPos ); 
					//pIAttachment->ProjectAttachment();
				}
			}
			pIAttachmentManager->ProjectAllAttachment();
*/

		}

		bool IsCPF = false;














		bool IsCHR = (0 == stricmp(fileExt,"chr"));
		if (IsCHR)
		{ 
			CLogFile::WriteLine("Loading Character Model...");











			{
				m_pCharacterBase = m_pAnimationSystem->CreateInstance( file );
				m_pCharacterAnim = m_pCharacterBase;
			}
     /*
			IAttachmentManager* pIAttachmentManager = m_pCompoundCharacter->GetIAttachmentManager();
			//STEP 1: create the socket for the face-attachment
			IAttachment* pIAttachment = pIAttachmentManager->CreateAttachment("C4_KaWummmmm",CA_FACE,0);
			//STEP 2: place the socket at the right position on the character
			if (pIAttachment) 
			{
				uint32 type = pIAttachment->GetType();
				if (type==CA_BONE || type==CA_FACE) 
				{
					//				<Attachment AName="back01" Type="CA_FACE" Rotation="-0.075126655,-0.99558783,0.03969615,   0.03981521" Position="0.041536111,-0.15446994,1.4762511" BoneName="" Binding="objects/characters/attachment/nanosuit/nanosuit_pouch.cgf" Flags="0" Material="objects/characters/attachment/nanosuit/Nanosuit_pockets"/>
					Quat WRot=Quat(-0.075126655f,-0.99558783f,0.03969615f,0.03981521f);
					Vec3 WPos=Vec3(0.041536111f,-0.15446994f,1.4762511f);
					pIAttachment->SetRMWRotation( WRot );
					pIAttachment->SetRMWPosition( WPos ); 
			//		pIAttachment->ProjectAttachment();
				}
			}
			pIAttachmentManager->ProjectAllAttachment();
*/

		}

		if (GetCharacterBase())
		{
			//m_pCompoundCharacter->AddRef();

			CThumbnailGenerator thumbGen;
			thumbGen.GenerateForFile( file );

			if (m_pAnimationBrowserPanel && m_pCharPanel_Animation && GetCharacterBase() != NULL && m_pCharacterAnim != NULL)
			{	
				UpdateAnimationList();
				UpdateBoneList();
			//SetCharacterUIInfo();		
			}

			f32	 radius	= GetCharacterBase()->GetAABB().GetRadius();		//m_pCompoundCharacter->GetRadius();
			Vec3 center	= GetCharacterBase()->GetAABB().GetCenter();		//m_pCompoundCharacter->GetCenter();

			m_AABB.min = center-Vec3(radius,radius,radius);
			m_AABB.max = center+Vec3(radius,radius,radius);
			if (!reload)
				m_camRadius = center.z + radius;

			m_Button_MOVE=0;
			m_Button_ROTATE=0;

			if (m_pMorphingDlg)
			{
				m_pMorphingDlg->m_DeformationSlider.SetValue(0);
				m_pMorphingDlg->m_DeformationNumber.SetValue(0);
				m_pMorphingDlg->m_DeformationSlider.EnableWindow(FALSE);
				m_pMorphingDlg->m_DeformationNumber.EnableWindow(FALSE);

				m_pMorphingDlg->m_button[0].EnableWindow(TRUE);
				m_pMorphingDlg->m_button[1].EnableWindow(TRUE);
				m_pMorphingDlg->m_button[2].EnableWindow(TRUE);
				m_pMorphingDlg->m_button[3].EnableWindow(TRUE);
				m_pMorphingDlg->m_button[4].EnableWindow(TRUE);
				m_pMorphingDlg->m_button[5].EnableWindow(TRUE);
				m_pMorphingDlg->m_button[6].EnableWindow(TRUE);
				m_pMorphingDlg->m_button[7].EnableWindow(TRUE);

				m_pMorphingDlg->m_button[0].SetCheck(FALSE);
				m_pMorphingDlg->m_button[1].SetCheck(FALSE);
				m_pMorphingDlg->m_button[2].SetCheck(FALSE);
				m_pMorphingDlg->m_button[3].SetCheck(FALSE);
				m_pMorphingDlg->m_button[5].SetCheck(FALSE);
				m_pMorphingDlg->m_button[6].SetCheck(FALSE);
				m_pMorphingDlg->m_button[7].SetCheck(FALSE);

				m_pMorphingDlg->m_button[0].SetBkColor(RGB(0x3f,0x3f,0x3f));
				m_pMorphingDlg->m_button[0].SetPushedBkColor(RGB(0x000,0x00,0x00));
				m_pMorphingDlg->m_button[1].SetBkColor(RGB(0x7f,0x3f,0x3f));
				m_pMorphingDlg->m_button[1].SetPushedBkColor(RGB(0x0ff,0x00,0x00));
				m_pMorphingDlg->m_button[2].SetBkColor(RGB(0x3f,0x7f,0x3f));
				m_pMorphingDlg->m_button[2].SetPushedBkColor(RGB(0x00,0xff,0x00));
				m_pMorphingDlg->m_button[3].SetBkColor(RGB(0x7f,0x7f,0x3f));
				m_pMorphingDlg->m_button[3].SetPushedBkColor(RGB(0xff,0xff,0x00));
				m_pMorphingDlg->m_button[4].SetBkColor(RGB(0x3f,0x3f,0x7f));
				m_pMorphingDlg->m_button[4].SetPushedBkColor(RGB(0x000,0x00,0xff));
				m_pMorphingDlg->m_button[5].SetBkColor(RGB(0x7f,0x3f,0x7f));
				m_pMorphingDlg->m_button[5].SetPushedBkColor(RGB(0xff,0x00,0xff));
				m_pMorphingDlg->m_button[6].SetBkColor(RGB(0x3f,0x7f,0x7f));
				m_pMorphingDlg->m_button[6].SetPushedBkColor(RGB(0x00,0xff,0xff));
				m_pMorphingDlg->m_button[7].SetBkColor(RGB(0x7f,0x7f,0x7f));
				m_pMorphingDlg->m_button[7].SetPushedBkColor(RGB(0xff,0xff,0xff));
				m_pMorphingDlg->m_nColor=9;
			}

			if (m_pAttachmentsDlg)
			{
				m_pAttachmentsDlg->CharacterChanged=0;
				m_pAttachmentsDlg->ReloadAttachment();
	//			m_pAttachmentsDlg->ClearBones();

				//initialize selection
				m_SelectedAttachment=0;
				IAttachmentManager* pAttachmentManager = GetCharacterBase()->GetIAttachmentManager();
				uint32 numAttachment = pAttachmentManager->GetAttachmentCount();
				if (numAttachment) 
				{
					IAttachment* pIAttachment = pAttachmentManager->GetInterfaceByIndex(m_SelectedAttachment);  
					uint32 type = pIAttachment->GetType();
					if (type==CA_BONE || type==CA_FACE) 
					{
						m_ArcBall.DragRotation.SetIdentity();
						m_ArcBall.ObjectRotation	=	pIAttachment->GetAttAbsoluteDefault().q;
						m_ArcBall.sphere.center		= pIAttachment->GetAttAbsoluteDefault().t; 
					}

					m_pAttachmentsDlg->UpdateList();

					string name = pIAttachment->GetName();
					uint32 n = m_pAttachmentsDlg->m_attachmentsList.FindString(-1,name);
					m_pAttachmentsDlg->m_attachmentsList.SetCurSel(n);
					m_pAttachmentsDlg->OnAttachmentSelect();
					m_pAttachmentsDlg->CharacterChanged=1;
				}
			}
		}	
		else if( !IsCPF && !IsCHR && !IsCGA && !IsCDF )
		{
			LoadStaticObject( file );
			m_pCharacterEditor->SetViewPaneTitle( file.GetBuffer(0) );
		}
	}
	else
	{
		MessageBox( "Preview of this file type not supported","Preview Error",MB_OK|MB_ICONEXCLAMATION );
		return;
	}

	//--------------------------------------------------------------------------------

//	m_objectAngles(0,0,0);
	m_camRadius = (m_AABB.max-m_AABB.min).GetLength();

	Matrix34 VMat;
	VMat.SetRotationZ(gf_PI);
	VMat.SetTranslation( Vec3(0, m_camRadius,(m_AABB.max.z+m_AABB.min.z)/2) );
	SetViewTM( VMat );

	if (m_pCharacterPropertiesDlg)
		m_pCharacterPropertiesDlg->SetMaterial(GetMaterial());

	Physicalize();

	if (m_pCharPanel_Animation)
		m_pCharPanel_Animation->SetBaseCharacter();

	//------------------------------------------------------------------------------
	// Save default material

	IMaterial *pMtl = 0;
	if (m_object)
		pMtl = m_object->GetMaterial();
	else if (GetCharacterBase())
		pMtl = GetCharacterBase()->GetMaterial();

	m_pDefaultMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(pMtl);

	SendOnCharacterChanged();
}






//////////////////////////////////////////////////////////////////////////
void CModelViewportCE::UpdateAnimationList()
{
	// Fill the combo box with the name of the animation sequences
	m_pCharPanel_Animation->ClearAnims();
	m_pCharPanel_Animation->DisableFileBroser();

	if ( !m_pCharacterAnim )
		return;

	CAnimationBrowser* pPanel = GetAnimBrowserPanel();
	if(pPanel)
		pPanel->UpdateAnimations(m_pCharacterAnim);
}


void CModelViewportCE::UpdateBoneList()
{
	if ( !m_pAttachmentsDlg )
		return;

	//------------------------------------------------------------------------------
	// Fill the bone list in the TabPanel
	//------------------------------------------------------------------------------
	//ICryCharModel *pCharModel = m_pCompoundCharacter->GetModel();
	uint32 numBones = GetCharacterBase()->GetISkeletonPose()->GetJointCount();
	m_pAttachmentsDlg->ClearBones();
	for (uint32 i=0; i < numBones; i++)
	{
		const char *str = GetCharacterBase()->GetISkeletonPose()->GetJointNameByID(i);
		if (str == NULL)
			break;
		if (strlen(str) > 0)
			m_pAttachmentsDlg->AddBone(str);
	}
	m_pAttachmentsDlg->SelectBone( m_attachBone );

}

//-----------------------------------------------------
void CModelViewportCE::OnAnimPlay()
{
	m_Button_MOVE		= 0;
	m_Button_ROTATE	= 0;
	GetCharacterBase()->SetResetMode(0);
	
	// TODO: Add your command handler code here
	if (m_pCharPanel_Animation && GetCharacterBase() != NULL)
	{
		// the name of the currently selected animation
		CString strAnimName = m_pCharPanel_Animation->GetCurrAnimName();
		PlayAnimation(strAnimName);
	}

	// Just play the first user selected motion if more than one motions are selected
	std::vector<CString>& anims = CUIEnumerations::GetUIEnumerationsInstance().GetSelectedAnimations();
	if(!anims.empty())
		PlayAnimation(anims[0]);
}

//-----------------------------------------------------
void CModelViewportCE::OnAnimForcePlay()
{
	CModelViewport::OnAnimForcePlay();
}

//////////////////////////////////////////////////////////////////////////
void CModelViewportCE::PlayAnimation(const char* szName)
{
	m_Button_MOVE		= 0;
	m_Button_ROTATE	= 0;
	GetCharacterBase()->SetResetMode(0);

	if (m_pCharPanel_Animation)
	{
		int layer = m_pCharPanel_Animation->GetLayer();

		bool bTransRot2000					= m_pCharPanel_Animation->GetAnimationDrivenMotion();

		bool bMirrorAnimation				= m_pCharPanel_Animation->GetMirrorAnimation();
		bool bLoopAnimation					= m_pCharPanel_Animation->GetLoopAnimation();
		bool bRepeatLastKey					= m_pCharPanel_Animation->GetRepeatLastKey();
		bool bVTimeWarping					= m_pCharPanel_Animation->GetVTimeWarping();
		bool bDisableMultilayer		= m_pCharPanel_Animation->GetDisableMultilayer();

		bool bAllowARestart					= m_pCharPanel_Animation->GetAllowAnimationRestart();

		bool bFVE										= m_pCharPanel_Animation->GetFootAnchoring();

		if (szName[0] == '#')
		{
			f32 trans=0.25;
			trans = m_pCharPanel_Animation->GetTransitionTimeValue();

			IFacialInstance* pFacialInstance = (m_pCharacterAnim ? m_pCharacterAnim->GetFacialInstance() : 0);
			IFacialModel* pFacialModel = (pFacialInstance ? pFacialInstance->GetFacialModel() : 0);
			IFacialEffectorsLibrary* pEffectorsLibrary = (pFacialModel ? pFacialModel->GetLibrary() : 0);
			IFacialEffector* pFacialEffector = (pEffectorsLibrary ? pEffectorsLibrary->Find(szName + 1) : 0);
			//if (pFacialInstance && pFacialEffector)
			//	pFacialInstance->StartEffectorChannel(pFacialEffector, 1.0, trans, 2.0f * trans);

			CryCharMorphParams Params;
			IMorphing* pIMorphing = m_pCharacterAnim->GetIMorphing();
			pIMorphing->StartMorph(szName,Params);
		}
		else
		{
			CryCharAnimationParams Params (layer);

			ISkeletonAnim* pISkeletonAnim = NULL;
			ISkeletonPose* pISkeletonPose = NULL;
			if(strlen(szName) != 0){
				pISkeletonAnim = m_pCharacterAnim->GetISkeletonAnim();
				pISkeletonPose = m_pCharacterAnim->GetISkeletonPose();
			}

			if (pISkeletonAnim)
			{
				pISkeletonAnim->SetAnimationDrivenMotion(bTransRot2000);
				pISkeletonPose->SetFootAnchoring( bFVE );

				//	if (bManualUpdate)
				//		Params.m_nFlags |= CA_MANUAL_UPDATE;
				if (bLoopAnimation)
					Params.m_nFlags |= CA_LOOP_ANIMATION;
				if (bRepeatLastKey)
					Params.m_nFlags |= CA_REPEAT_LAST_KEY;

				if (bVTimeWarping)
					Params.m_nFlags |= CA_TRANSITION_TIMEWARPING;
				if (bDisableMultilayer)
					Params.m_nFlags |= CA_DISABLE_MULTILAYER;

				if (bAllowARestart)
					Params.m_nFlags |= CA_ALLOW_ANIM_RESTART;

				//		Params.m_nFlags |= CA_START_AFTER;

				Params.m_fTransTime=m_pCharPanel_Animation->GetTransitionTimeValue();

				if (mv_showSuperimposed)
					pISkeletonPose->SetForceSkeletonUpdate(3);


				pISkeletonPose->SetSuperimposed(mv_showSuperimposed);

				uint32 IsNULL = (szName[0]=='n' && szName[1]=='u' && szName[2]=='l' && szName[3]=='l');
				if (IsNULL==0)
				{
					IAnimationSet* pAnimations = m_pCharacterAnim->GetIAnimationSet();
					int32 nAnimID = pAnimations->GetAnimIDByName(szName);
					if (nAnimID>=0)
					{
						uint32 flags = pAnimations->GetAnimationFlags(nAnimID);
						uint32 loaded = flags & CA_ASSET_LOADED;

						pISkeletonAnim->StartAnimation( szName, Params);
					}
				}
			}
		}
	}
}



//-------------------------------------------------------------------
void CModelViewportCE::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
}
//-------------------------------------------------------------------
void CModelViewportCE::OnRButtonDown( UINT nFlags, CPoint point)
{
	m_MouseButtonR = true;
	CRenderViewport::OnRButtonDown(nFlags,point);
}
//-------------------------------------------------------------------
void CModelViewportCE::OnRButtonUp( UINT nFlags, CPoint point)
{
	m_MouseButtonR = false;
	CRenderViewport::OnRButtonUp(nFlags,point);
}

//-------------------------------------------------------------------

void CModelViewportCE::OnLButtonDown( UINT nFlags, CPoint point)
{
	ICharacterInstance* pCharacter=GetCharacterBase();
	if (pCharacter)
	{
		if (pCharacter->GetResetMode()) 
		{
			IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();

			m_opMode = SelectMode;

			m_MouseButtonL = true;
			m_pAttachmentsDlg->CharacterChanged=1;

			m_cMouseDownPos = point;

			if (m_Button_MOVE)
			{
				Matrix34 m34=Matrix34(Matrix33(m_ArcBall.ObjectRotation),m_ArcBall.sphere.center );
				m_HitContext.view = this;
				m_HitContext.b2DViewport = false;
				m_HitContext.point2d = point;
				ViewToWorldRay( point, m_HitContext.raySrc,m_HitContext.rayDir );
				m_HitContext.distanceTolerance = 0;
				if (m_AxisHelper.HitTest(m34,m_HitContext))
				{
					SetAxisConstrain( m_HitContext.axis );
					GetIEditor()->SetAxisConstrains( (AxisConstrains)m_HitContext.axis );
					if (m_Button_MOVE)
						m_opMode = MoveMode;
					SetConstructionMatrix( COORDS_LOCAL,m34 );
					return;
				}
			}


			if (m_MouseOnAttachment >= 0)
			{
				uint32 numAttachment = pIAttachmentManager->GetAttachmentCount();
				assert(m_MouseOnAttachment<numAttachment);

				m_SelectedAttachment=m_MouseOnAttachment;

				if (numAttachment) 
				{
					IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(m_SelectedAttachment);  
					uint32  type = pIAttachment->GetType(); 
					if (type==CA_BONE || type==CA_FACE ) 
					{
						m_ArcBall.DragRotation.SetIdentity();
						m_ArcBall.ObjectRotation	=	pIAttachment->GetAttAbsoluteDefault().q;
						m_ArcBall.sphere.center		= pIAttachment->GetAttAbsoluteDefault().t; 
					}
					string name = pIAttachment->GetName();
					uint32 n = m_pAttachmentsDlg->m_attachmentsList.FindString(-1,name);
					m_pAttachmentsDlg->m_attachmentsList.SetCurSel(n);
					m_pAttachmentsDlg->OnAttachmentSelect();
					m_pAttachmentsDlg->CharacterChanged=1;
				}
			}

			Matrix34 m34=Matrix34(Matrix33(m_ArcBall.ObjectRotation),m_ArcBall.sphere.center );
			SetConstructionMatrix( COORDS_LOCAL,m34 );
		}
	}

}

void CModelViewportCE::OnLButtonUp( UINT nFlags, CPoint point) {
	m_MouseButtonL = false;
	m_opMode = SelectMode;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

f32 CModelViewportCE::CharacterPicking( const Ray& mray ) 
{
	if (!m_pFaces)
		return 0;
	
	//IRenderer* renderer = GetIEditor()->GetRenderer();
	//IRenderAuxGeom* pAuxGeom = renderer->GetIRenderAuxGeom();
	//SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
	//renderFlags.SetFillMode( e_FillModeWireframe );
	//renderFlags.SetDrawInFrontMode( e_DrawInFrontOn );
	//pAuxGeom->SetRenderFlags( renderFlags );

	f32 distance=3.3E38f;

	/*
	uint32 numFaces = m_numFaces;
	for(uint32 i=0; i<numFaces; i++) 
	{
		uint32 i0=(m_pFaces)[i].i0;
		uint32 i1=(m_pFaces)[i].i1;
		uint32 i2=(m_pFaces)[i].i2;
		Vec3 output;

		Vec3 v0=(m_pSkinVertices)[i1].wpos1;
		Vec3 v1=(m_pSkinVertices)[i0].wpos1;
		Vec3 v2=(m_pSkinVertices)[i2].wpos1;
		bool t = Intersect::Ray_Triangle( mray, v0,v1,v2, output );
		if (t) 
		{
			//pAuxGeom->DrawLine( v0,RGBA8(0xff,0x00,0x00,0x00), v1,RGBA8(0x00,0xff,0x00,0x00) );
			//pAuxGeom->DrawLine( v1,RGBA8(0x00,0xff,0x00,0x00), v2,RGBA8(0x00,0x00,0xff,0x00) );
			//pAuxGeom->DrawLine( v2,RGBA8(0x00,0x00,0xff,0x00), v0,RGBA8(0xff,0x00,0x00,0x00) );
			f32 d = (output-m_Camera.GetPosition()).GetLength();
			if (distance>d) distance=d;
		}
	}
	*/

	ICharacterInstance* pICharacterInstance = GetCharacterBase();

	if (!pICharacterInstance)
		return distance;


	IRenderMesh* pRenderMesh = pICharacterInstance->GetICharacterModel()->GetRenderMesh(-1);//pIStaticObject->GetRenderMesh();
	if (pRenderMesh==0)
		return 0;

	uint32 NumVertices = pRenderMesh->GetVerticesCount();

	int pIndicesCount = pRenderMesh->GetIndicesCount();
	uint16* pIndices = pRenderMesh->GetIndexPtr(FSL_READ);
	int PosStride=0;
	byte* pVertices_strided = pRenderMesh->GetPosPtr(PosStride, FSL_READ);

	static Vec3 LineVertices[0x5000];
	for(uint32 i=0; i<NumVertices; i++) 
	{
		LineVertices[i] = *(Vec3*)pVertices_strided;
		LineVertices[i] = /*m34**/LineVertices[i];
		pVertices_strided += PosStride;
	}

	for(uint32 i=0; i<pIndicesCount; i=i+3) 
	{
		uint32 i0=pIndices[i+0];
		uint32 i1=pIndices[i+1];
		uint32 i2=pIndices[i+2];
		Vec3 output;
		bool t = Intersect::Ray_Triangle( mray, LineVertices[i1],LineVertices[i0],LineVertices[i2], output );
		if (t) 
		{	
			f32 d = (output-m_Camera.GetPosition()).GetLength();	
			if (distance>d) 
				distance=d;	
		}
	}

	return distance;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

f32 CModelViewportCE::AttachmentPicking( const Ray& mray, IAttachment* pIAttachment, const Matrix34& m34 ) 
{
	f32 distance = 3.3E38f;
	Vec3 output(ZERO);
	IAttachmentObject* pBindable = pIAttachment->GetIAttachmentObject();
	if (pBindable) 
	{

		AABB caabb=pBindable->GetAABB();
		OBB obb = OBB::CreateOBBfromAABB( Matrix33(m34), caabb );

		uint32 intersection = Intersect::Ray_OBB(mray,m34.GetTranslation(),obb,output);

		if (intersection) 
		{
			IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();

			IStatObj* pIStaticObject = pIAttachmentObject->GetIStatObj();
			if (pIStaticObject) 
			{
				uint32 count = pIStaticObject->GetSubObjectCount();
				if (count==0)
				{
					IRenderMesh* pRenderMesh = pIStaticObject->GetRenderMesh();
					if (pRenderMesh)
					{
						uint32 NumVertices = pRenderMesh->GetVerticesCount();

						int pIndicesCount = pRenderMesh->GetIndicesCount();
						uint16* pIndices = pRenderMesh->GetIndexPtr(FSL_READ);
						int PosStride=0;
						byte* pVertices_strided = pRenderMesh->GetPosPtr(PosStride, FSL_READ);

						static Vec3 LineVertices[0x5000];
						for(uint32 i=0; i<NumVertices; i++) 
						{
							LineVertices[i] = *(Vec3*)pVertices_strided;
							LineVertices[i] = m34*LineVertices[i];
							pVertices_strided += PosStride;
						}

						for(uint32 i=0; i<pIndicesCount; i=i+3) 
						{
							uint32 i0=pIndices[i+0];
							uint32 i1=pIndices[i+1];
							uint32 i2=pIndices[i+2];
							Vec3 output;
							bool t = Intersect::Ray_Triangle( mray, LineVertices[i1],LineVertices[i0],LineVertices[i2], output );
							if (t) {	f32 d = (output-m_Camera.GetPosition()).GetLength();	if (distance>d) distance=d;	}
						}
					}
				}
			}
			else 
			{
				//maybe object is an attached character 
				/*
			
				if (pICharacterInstance) 
				{
					ExtSkinVertex* pSkinVertices=0;
					uint32 numVertices=0;
					TFace* pFaces=0;
					uint32 numFaces=0;

					uint32 res = 0;//pICharacterInstance->GetMesh( pSkinVertices,numVertices, pFaces,numFaces);
					if (res)
					{
						//nt32 numFaces = numFaces;
						for(uint32 i=0; i<numFaces; i++) 
						{
							uint32 i0=pFaces[i].i0;
							uint32 i1=pFaces[i].i1;
							uint32 i2=pFaces[i].i2;
							Vec3 v0=m34*pSkinVertices[i1].wpos1;
							Vec3 v1=m34*pSkinVertices[i0].wpos1;
							Vec3 v2=m34*pSkinVertices[i2].wpos1;
							Vec3 output;
							bool t = Intersect::Ray_Triangle( mray, v0,v1,v2, output );
							if (t) 
							{
								f32 d = (output-m_Camera.GetPosition()).GetLength();	
								if (distance>d) distance=d;	
							}
						}
					}
					*/
				ICharacterInstance* pICharacterInstance = pIAttachmentObject->GetICharacterInstance();

				if (!pICharacterInstance)
					return distance;


				IRenderMesh* pRenderMesh = pICharacterInstance->GetICharacterModel()->GetRenderMesh(-1);//pIStaticObject->GetRenderMesh();
				uint32 NumVertices = pRenderMesh->GetVerticesCount();

				int pIndicesCount = pRenderMesh->GetIndicesCount();
				uint16* pIndices = pRenderMesh->GetIndexPtr(FSL_READ);
				int PosStride=0;
				byte* pVertices_strided = pRenderMesh->GetPosPtr(PosStride, FSL_READ);

				static Vec3 LineVertices[0x5000];
				for(uint32 i=0; i<NumVertices; i++) 
				{
					LineVertices[i] = *(Vec3*)pVertices_strided;
					LineVertices[i] = m34*LineVertices[i];
					pVertices_strided += PosStride;
				}

				for(uint32 i=0; i<pIndicesCount; i=i+3) 
				{
					uint32 i0=pIndices[i+0];
					uint32 i1=pIndices[i+1];
					uint32 i2=pIndices[i+2];
					Vec3 output;
					bool t = Intersect::Ray_Triangle( mray, LineVertices[i1],LineVertices[i0],LineVertices[i2], output );
					if (t) 
					{	
						f32 d = (output-m_Camera.GetPosition()).GetLength();	
						if (distance>d) 
							distance=d;	
					}
				}
			}
		}	 //if mouse on obb
	} 
	else 
	{
		AABB caabb = AABB(Vec3( -0.05f, -0.05f, -0.05f),Vec3( +0.05f, +0.05f, +0.05f));
		OBB obb = OBB::CreateOBBfromAABB( Matrix33(Quat(1,0,0,0)), caabb );
		uint32 intersection = Intersect::Ray_OBB(mray,m34.GetTranslation(),obb,output);
		if (intersection) {	distance = (output-m_Camera.GetPosition()).GetLength();	}
	}

	return distance;
}



//-------------------------------------------------------------------

void CModelViewportCE::OnMouseMove(UINT nFlags, CPoint point)
{
	CRenderViewport::OnMouseMove(nFlags,point);

	//calculate the Ray for camera-pos to mouse-cursor
	GetWindowRect(m_WinRect);
	f32 RresX=(f32)(m_WinRect.right-m_WinRect.left); 
	f32 RresY=(f32)(m_WinRect.bottom-m_WinRect.top);
	f32 MiddleX=RresX/2.0f;
	f32 MiddleY=RresY/2.0f;
	Vec3 MoursePos3D = Vec3( point.x-MiddleX, m_Camera.GetEdgeP().y, -point.y+MiddleY ) * Matrix33(m_Camera.GetViewMatrix()) + m_Camera.GetPosition();
	Ray mray(m_Camera.GetPosition(),(MoursePos3D-m_Camera.GetPosition()).GetNormalized() );

	if (m_MouseButtonR==false && GetCharacterBase()  != NULL && m_SelectionUpdate && GetCharacterBase()->GetResetMode()) 
	{
		m_SelectionUpdate=0;
		ClosestPoint ct;

		//-----------------------------------------------------------------------
		//---            find closest point on character-mesh                 ---
		//-----------------------------------------------------------------------
		IAttachmentManager* pIAttachmentManager = GetCharacterBase()->GetIAttachmentManager();
		uint32 numAttachment = pIAttachmentManager->GetAttachmentCount();

		uint32 res = 0;//m_pCompoundCharacter->GetMesh( m_pSkinVertices,m_numSkinVertices, m_pFaces,m_numFaces);

		ct.basemodel=0;
		//if (res)
			ct.distance = CharacterPicking( mray ); 
		if (ct.distance<10000.0f)
			ct.basemodel=1;
		//-----------------------------------------------------------------------
		//---            find closest point on arcball                        ---
		//-----------------------------------------------------------------------
		if (m_Button_ROTATE && numAttachment>0) 
		{
			Vec3 output;
			uint32 i=Intersect::Ray_SphereFirst(mray,m_ArcBall.sphere,output);
			if (i) 
			{ 
				f32 d=(output-m_Camera.GetPosition()).GetLength();
				if (ct.distance>d) 
				{
					ct.basemodel=0;
					ct.distance=d;
				}
				//ct.distance=0;
				for (uint32 s=0; s<numAttachment; s++)  
				{
					IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(s);
					uint32 type = pIAttachment->GetType();
					if (type==CA_BONE || type==CA_FACE) 
						if (pIAttachment->GetAttAbsoluteDefault().t==m_ArcBall.sphere.center)	ct.selection =s;
				} //for loop
			}
		}



		//----------------------------------------------------------------------
		//----------------------------------------------------------------------
		//----------------------------------------------------------------------

		SetCurrentCursor( STD_CURSOR_DEFAULT,"" );

		if (numAttachment) 
		{
			if (m_Button_ROTATE) 
			{
				m_ArcBall.ArcControl( mray, m_MouseButtonL );
			}

			if (m_Button_MOVE) 
			{
				Matrix34 m34=Matrix34(Matrix33(m_ArcBall.ObjectRotation),m_ArcBall.sphere.center );
				m_HitContext.view = this;
				m_HitContext.b2DViewport = false;
				m_HitContext.point2d = point;
				m_HitContext.rayDir = mray.direction;
				m_HitContext.raySrc = mray.origin;
				ViewToWorldRay( point, m_HitContext.raySrc,m_HitContext.rayDir );
				m_HitContext.distanceTolerance = 0;
				if (m_AxisHelper.HitTest(m34,m_HitContext))
				{
					SetCurrentCursor( STD_CURSOR_MOVE,"Attachment" );
					ct.distance=0;						
					for (uint32 s=0; s<numAttachment; s++)  
					{
						IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(s);
						uint32 type = pIAttachment->GetType();
						if (type==CA_BONE || type==CA_FACE) 
							if (pIAttachment->GetAttAbsoluteDefault().t==m_ArcBall.sphere.center)	ct.selection =s;
					} //for loop
				}
			}
		}



		//-----------------------------------------------------------------------
		//---       loop over all attachments and do raycasting               ---
		//-----------------------------------------------------------------------
		m_MouseOnAttachment=-1;
		for (uint32 s=0; s<numAttachment; s++)  
		{
			IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(s);
			uint32 type = pIAttachment->GetType();
			//if (type==CA_BONE || type==CA_FACE) 
			{
				Matrix34 m34=Matrix34( pIAttachment->GetAttAbsoluteDefault() );
				if (type==CA_SKIN) 
					m34.SetIdentity(); 
				f32 distance = AttachmentPicking(mray, pIAttachment,m34); 
				if (ct.distance>distance) 
				{
					ct.basemodel	=	0;
					ct.distance		= distance;
					ct.selection	=s;
				}
			} 

		} //for loop
		cp=ct;
		m_MouseOnAttachment=cp.selection;

		if (m_opMode == MoveMode)
		{
			Vec3 p1 = MapViewToCP(m_cMouseDownPos);
			Vec3 p2 = MapViewToCP(point);
			if (p1.IsZero() || p2.IsZero())
				return;
			Vec3 v = GetCPVector(p1,p2);

			m_cMouseDownPos = point;
			m_ArcBall.sphere.center += v;
		}
	}
}

void CModelViewportCE::OnRender()
{
	IRenderer* renderer = GetIEditor()->GetRenderer();
	IRenderAuxGeom* pAuxGeom = renderer->GetIRenderAuxGeom();

	if (m_AxisHelper.GetHighlightAxis() == 0)
		m_AxisHelper.SetHighlightAxis( GetAxisConstrain() );

	if (GetCharacterBase()) 
	{	

		if (mv_showJointsValues)
		{
			CString bonename;
			if (m_pAttachmentsDlg)
				bonename = m_pAttachmentsDlg->GetBonenameFromWindow();
			int32 idx = GetCharacterBase()->GetISkeletonPose()->GetJointIDByName (bonename);

			if (idx>=0)
			{		
				uint32 ypos = 300;
				float color1[4] = {1,1,1,1};
				renderer->Draw2dLabel(12,ypos,1.2f,color1,false,"bonename: %s %d",bonename,idx);
				ypos+=10;

				Matrix34 abs34=Matrix34( GetCharacterBase()->GetISkeletonPose()->GetAbsJointByID(idx) );
				Matrix34 rel34=Matrix34( GetCharacterBase()->GetISkeletonPose()->GetRelJointByID(idx) );

				Vec3 WPos			=	abs34.GetTranslation();
				Vec3 RPos			= rel34.GetTranslation();
				Vec3 AxisX		=	abs34.GetColumn0();
				Vec3 AxisY		=	abs34.GetColumn1();
				Vec3 AxisZ		=	abs34.GetColumn2();

				SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
				pAuxGeom->SetRenderFlags( renderFlags );
				pAuxGeom->DrawLine( WPos,RGBA8(0xff,0x00,0x00,0x00), WPos+AxisX*29.0f,RGBA8(0x00,0x00,0x00,0x00) );
				pAuxGeom->DrawLine( WPos,RGBA8(0x00,0xff,0x00,0x00), WPos+AxisY*29.0f,RGBA8(0x00,0x00,0x00,0x00) );
				pAuxGeom->DrawLine( WPos,RGBA8(0x00,0x00,0xff,0x00), WPos+AxisZ*29.0f,RGBA8(0x00,0x00,0x00,0x00) );

				QuatT absQuat=QuatT(abs34);
				QuatT relQuat=QuatT(rel34);
				renderer->Draw2dLabel(12,ypos,1.2f,color1,false,"absQuat: %15.10f %15.10f %15.10f %15.10f  --- %15.10f %15.10f %15.10f",absQuat.q.w,absQuat.q.v.x,absQuat.q.v.y,absQuat.q.v.z,  absQuat.t.x,absQuat.t.y,absQuat.t.z);
				ypos+=10;
				renderer->Draw2dLabel(12,ypos,1.2f,color1,false,"relQuat: %15.10f %15.10f %15.10f %15.10f  --- %15.10f %15.10f %15.10f",relQuat.q.w,relQuat.q.v.x,relQuat.q.v.y,relQuat.q.v.z,  relQuat.t.x,relQuat.t.y,relQuat.t.z );
				ypos+=10;
				renderer->Draw2dLabel(12,ypos,1.2f,color1,false,"WPos: %15.10f %15.10f %15.10f",WPos.x,WPos.y,WPos.z);
				ypos+=10;
				renderer->Draw2dLabel(12,ypos,1.2f,color1,false,"RPos: %15.10f %15.10f %15.10f",RPos.x,RPos.y,RPos.z);
				ypos+=10;
			}
		}

		if (mv_showJointNames)
		{
			ISkeletonPose* pSkeletonPose = (GetCharacterBase() ? GetCharacterBase()->GetISkeletonPose() : 0);
			for (int jointIndex = 0, jointCount = (pSkeletonPose ? pSkeletonPose->GetJointCount() : 0); jointIndex < jointCount; ++jointIndex)
			{
				const char* jointName = (pSkeletonPose ? pSkeletonPose->GetJointNameByID(jointIndex) : 0);
				QuatT jointTM = (pSkeletonPose ? pSkeletonPose->GetAbsJointByID(jointIndex) : QuatT(IDENTITY));
				if (renderer)
					renderer->DrawLabel(jointTM.t, 1, "%s", (jointName ? jointName : "<UNKOWN JOINT>"));
			}
		}

		//------------------------------------------------------------------------------
		// PMG aim-pose and IK-pose testing code
		if(0)
		{		
			float color1[4] = {1,1,1,1};

			ISkeletonPose* pSkeletonPose = (GetCharacterBase() ? GetCharacterBase()->GetISkeletonPose() : 0);
			for (int jointIndex = 0, jointCount = (pSkeletonPose ? pSkeletonPose->GetJointCount() : 0); jointIndex < jointCount; ++jointIndex)
			{
				const char* jointName = (pSkeletonPose ? pSkeletonPose->GetJointNameByID(jointIndex) : 0);
				QuatT jointTM = (pSkeletonPose ? pSkeletonPose->GetAbsJointByID(jointIndex) : QuatT(IDENTITY));

				if(stricmp(jointName, "weapon_bone") == 0)
				{
					ISkeletonAnim* pISkeletonAnim = GetCharacterAnim()->GetISkeletonAnim();
					Vec3 controlParam = Vec3(ZERO);
					if(pISkeletonAnim)
					{
						controlParam.x = pISkeletonAnim->GetControlParam("IK_POSITION_X");
						controlParam.y = pISkeletonAnim->GetControlParam("IK_POSITION_Y");
						controlParam.z = pISkeletonAnim->GetControlParam("IK_POSITION_Z");
					}
					Vec3 posDiff = jointTM.t - controlParam;

					renderer->Draw2dLabel(600,50,1.2f,color1,false,"Weapon-bone position diff: %.3f, %.3f, %.3f", posDiff.x, posDiff.y, posDiff.z);					
					
					//------------------------------------------------------------------------------
					// Aimpose diff
					Quat qAbs = jointTM.q;
					Vec3 aimPosVec = qAbs.GetColumn1(); // Y axis
					aimPosVec.GetNormalized();

					f32 px = atan2f( aimPosVec.y, aimPosVec.x);
					if( px >= -gf_PI && px <= -gf_PI / 2.0f - 0.15f )
						px += 2*gf_PI;
					f32 py = acos( aimPosVec.z );

					if(pISkeletonAnim)
					{
						controlParam.x = pISkeletonAnim->GetControlParam("AIMPOSE_XY_ANGLE");
						controlParam.y = pISkeletonAnim->GetControlParam("AIMPOSE_YZ_ANGLE");
					}
					renderer->Draw2dLabel(600,100,1.2f,color1,false,"Aimpose Diff: %.3f, %.3f", px - controlParam.x, py - controlParam.y);
				}
			}
		}

		//-------------------------------------------------------------------------------------
		//-------------------------------------------------------------------------------------
		//-------------------------------------------------------------------------------------
		if (m_pCharPanel_Animation)
		{
			if (m_Button_ROTATE || m_Button_MOVE) 
			{
				m_pCharPanel_Animation->m_PlayControl.SetCheck(0);
				m_pCharPanel_Animation->m_PlayControl.EnableWindow(FALSE);
				m_pCharPanel_Animation->m_FixedCamera.SetCheck(0);
				m_pCharPanel_Animation->m_FixedCamera.EnableWindow(FALSE);

				m_pCharPanel_Animation->m_PathFollowing.SetCheck(0);
				m_pCharPanel_Animation->m_PathFollowing.EnableWindow(FALSE);
				m_pCharPanel_Animation->m_AttachedCamera.SetCheck(0);
				m_pCharPanel_Animation->m_AttachedCamera.EnableWindow(FALSE);

				m_pCharPanel_Animation->m_Idle2Move.SetCheck(0);
				m_pCharPanel_Animation->m_Idle2Move.EnableWindow(FALSE);

				m_pCharPanel_Animation->m_IdleStep.SetCheck(0);
				m_pCharPanel_Animation->m_IdleStep.EnableWindow(FALSE);
			}
			else
			{
				m_pCharPanel_Animation->m_PlayControl.EnableWindow(TRUE);
				m_pCharPanel_Animation->m_FixedCamera.EnableWindow(TRUE);
				m_pCharPanel_Animation->m_PathFollowing.EnableWindow(TRUE);
				m_pCharPanel_Animation->m_AttachedCamera.EnableWindow(TRUE);

				m_pCharPanel_Animation->m_Idle2Move.EnableWindow(TRUE);
				m_pCharPanel_Animation->m_IdleStep.EnableWindow(TRUE);
			}
		}


/*		static f32 m_fLastAnimUpdateTime=0.0f;
		f32 m_fFrameTime=0;
		f32 fAnimUpdateTime = GetIEditor()->GetSystem()->GetITimer()->GetCurrTime();

		if (m_fLastAnimUpdateTime > 0)
			m_fFrameTime = (fAnimUpdateTime - m_fLastAnimUpdateTime)*32;

		if (m_fFrameTime >= 0) 
		{
			m_fLastAnimUpdateTime = fAnimUpdateTime;
			if (m_fFrameTime>1.0f) 
				m_fFrameTime=1.0f;

			extern f32 OldVal;
			extern f32 NewVal;
			f32 dif = (NewVal-OldVal);
			f32 d=dif*dif;
			if (dif<0)
				d*=-1;

			OldVal += d*m_fFrameTime;
			if (OldVal>1)
				OldVal=1;

			CryCharMorphParams Params;
			Params.m_fBlendIn		=	1;
			Params.m_fLength		=	9.000000f;//(fBlendInTime+fBlendOutTime)/2;
			Params.m_fBlendOut	=	1;
			Params.m_fAmplitude = m_pCharPanel_Animation->GetMTAmplitude();
			Params.m_fStartTime = OldVal*1.5f;
			if (Params.m_fStartTime<0) 
				Params.m_fStartTime=0;
			if (Params.m_fStartTime>1) 
				Params.m_fStartTime=1;
			Params.m_fSpeed = 1;
			m_pCharacterAnim->StopMorph ("#test_1",1);
			m_pCharacterAnim->StartMorph("#test_1", Params);

			Params.m_fStartTime = (OldVal-0.5f)*1.7f;
			if (Params.m_fStartTime<0) 
				Params.m_fStartTime=0;
			if (Params.m_fStartTime>1) 
				Params.m_fStartTime=1;
		//	m_pCharacterAnim->StopMorph ("#Full_Hurt",1);
		//	m_pCharacterAnim->StartMorph("#Full_Hurt", Params);
		//	m_pCharacterAnim->StopMorph ("#Asian_Head_Not_Happy",1);
		//	m_pCharacterAnim->StartMorph("#Asian_Head_Not_Happy", Params);
			Params.m_fAmplitude = 0.40f; //m_pCharPanel_Animation->GetMTAmplitude();
			m_pCharacterAnim->StopMorph ("#test_2",1);
			m_pCharacterAnim->StartMorph("#test_2", Params);


			Params.m_fStartTime = (1-OldVal*2);
			if (Params.m_fStartTime<0) 
				Params.m_fStartTime=0;
			if (Params.m_fStartTime>1) 
				Params.m_fStartTime=1;
			Params.m_fAmplitude = 0.750f; //m_pCharPanel_Animation->GetMTAmplitude();
			m_pCharacterAnim->StopMorph ("#Asian_Head_Not_Happy",1);
			m_pCharacterAnim->StartMorph("#Asian_Head_Not_Happy", Params);

		}*/


		//m_pCompoundCharacter->SetScale(Vec3(-1,2,1));
		ICharacterInstance* charBase = GetCharacterBase();
		uint32 resetMode = 0;
		if(charBase)
			resetMode = charBase->GetResetMode();

		if (resetMode) 
		{
			//-----------------------------------------------------------------------
			//---                    resolve selection                            ---
			//-----------------------------------------------------------------------
			IAttachmentManager* pIAttachmentManager = GetCharacterBase()->GetIAttachmentManager();
			uint32 numAttachment=pIAttachmentManager->GetAttachmentCount();
			if (numAttachment==0) 
			{
				m_SelectedAttachment=-1;
			}
			if (numAttachment==1) 
			{

				m_SelectedAttachment=0;
			}

			//-----------------------------------------------------------------------
			// draw wireframe over closest selection
			//-----------------------------------------------------------------------
			if (cp.selection != -1) 
			{
				IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(cp.selection);
				IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();

				if (pIAttachmentObject) 
				{
					IStatObj* pIStaticObject = pIAttachmentObject->GetIStatObj();
					if (pIStaticObject) 
					{
						uint32 count = pIStaticObject->GetSubObjectCount();
						if (count==0)
						{
							IRenderMesh* pRenderMesh = pIStaticObject->GetRenderMesh();
							if (pRenderMesh)
							{
								uint32 NumVertices = pRenderMesh->GetVerticesCount();

								int numIndices = pRenderMesh->GetIndicesCount();
								uint16* pIndices = pRenderMesh->GetIndexPtr(FSL_READ);
								int PosStride=0;
								byte* pVertices_strided = pRenderMesh->GetPosPtr(PosStride, FSL_READ);

								Matrix34 m34 = Matrix34( pIAttachment->GetAttAbsoluteDefault() );
								static Vec3 LineVertices[0x5000];
								for(uint32 i=0; i<NumVertices; i++) 
								{
									LineVertices[i]		= *(Vec3*)pVertices_strided;
									LineVertices[i]		= m34*LineVertices[i];
									pVertices_strided += PosStride;
								}
								SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
								renderFlags.SetFillMode( e_FillModeWireframe );
								renderFlags.SetDrawInFrontMode( e_DrawInFrontOn );
								renderFlags.SetAlphaBlendMode(e_AlphaAdditive);
								pAuxGeom->SetRenderFlags( renderFlags );
								pAuxGeom->DrawTriangles(LineVertices,NumVertices,pIndices,numIndices,RGBA8(0x00,0x1f,0x00,0x00));		
							}
						}
					} 
					else 
					{
						//maybe object is an attached character 
						ICharacterInstance* pICharacterInstance = pIAttachmentObject->GetICharacterInstance();
						if (pICharacterInstance) 
						{
							Matrix34 m34 = Matrix34( pIAttachment->GetAttAbsoluteDefault() );
							pICharacterInstance->DrawWireframeStatic(m34,0,RGBA8(0x00,0x1f,0x00,0x00));
						}
					}
				}
			} 
			else 
			{
				if (cp.basemodel) 
				{
					GetCharacterBase()->DrawWireframeStatic( Matrix34(IDENTITY),0,RGBA8(0x1f,0x00,0x00,0x00));
				}
			}


			//-----------------------------------------------------------------------
			//---            use arcball to re-orient attachment                  ---
			//-----------------------------------------------------------------------
			if (numAttachment) 
			{
				if (m_SelectedAttachment != -1) 
				{
					IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(m_SelectedAttachment);
					if (pIAttachment) 
					{
						uint32 type = pIAttachment->GetType();
						if (type==CA_BONE || type==CA_FACE) 
						{
							//<Attachment AName="back01" Type="CA_FACE" Rotation="-0.075126655,-0.99558783,0.03969615,   0.03981521" Position="0.041536111,-0.15446994,1.4762511" BoneName="" Binding="objects/characters/attachment/nanosuit/nanosuit_pouch.cgf" Flags="0" Material="objects/characters/attachment/nanosuit/Nanosuit_pockets"/>
							Quat WRot=m_ArcBall.DragRotation*m_ArcBall.ObjectRotation;
							Vec3 WPos=m_ArcBall.sphere.center;
							pIAttachment->SetAttAbsoluteDefault( QuatT(WRot,WPos) );

						//	uint32 ypos = 300;
						//	float color1[4] = {1,1,1,1};
						//	renderer->Draw2dLabel(12,ypos,1.2f,color1,false,"Attachment rotation: %15.10f (%15.10f %15.10f %15.10f)", WRot.w, WRot.v.x,WRot.v.y,WRot.v.z );
						//	ypos+=10;
						//	renderer->Draw2dLabel(12,ypos,1.2f,color1,false,"Attachment position: %15.10f %15.10f %15.10f", WPos.x, WPos.y, WPos.z);
						//	ypos+=10;

							pIAttachment->ProjectAttachment();

							if (m_Button_ROTATE) 
							{
								m_ArcBall.Draw_Sphere( GetCamera(),pAuxGeom );
							}
							if (m_Button_MOVE) 
							{
								SAuxGeomRenderFlags renderFlags( e_Def3DPublicRenderflags );
								renderFlags.SetFillMode( e_FillModeSolid );
								pAuxGeom->SetRenderFlags( renderFlags );
								Matrix34 m34=Matrix34(Matrix33(m_ArcBall.DragRotation*m_ArcBall.ObjectRotation),m_ArcBall.sphere.center );
								m_AxisHelper.DrawAxis(m34,m_displayContext);
							}
						}
					}
				}
			}


		} //if resetmode
	} //if character

	m_SelectionUpdate=1;
	CModelViewport::OnRender();

	if (m_pAnimationControlDlg != 0)
		m_pAnimationControlDlg->Update();

}

void CModelViewportCE::SetCharacterChangeListener(ICharacterChangeListener* pListener)
{
	m_pCharacterChangeListener = pListener;
}

void CModelViewportCE::SendOnCharacterChanged()
{

	if ( m_pCharacterChangeListener )
		m_pCharacterChangeListener->OnCharacterChanged();
}

void CModelViewportCE::Update() 
{	
	if( m_pCharacterEditor )
  {
    m_pCharacterEditor->SceneUpdate();
  }

  //
  CModelViewport::Update();	
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CModelViewportCE::ExternalPostProcessing(ICharacterInstance* pInstance)
{
	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();

	//	float color1[4] = {1,1,1,1};
	//	m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"callback Radiant: %f %f %f",offset.t.x,offset.t.y,offset.t.z );
	//	g_ypos+=10;

	if (m_pCharPanel_Animation)
	{
		uint32 GroundAlign = m_pCharPanel_Animation->GetGroundAlign();

		uint32 LFootIK = m_pCharPanel_Animation->GetLFootIK();
		uint32 RFootIK = m_pCharPanel_Animation->GetRFootIK();
		uint32 LArmIK = m_pCharPanel_Animation->GetLArmIK();
		uint32 RArmIK = m_pCharPanel_Animation->GetRArmIK();

		if (GroundAlign)
			UseGroundAlignTest(pInstance);  //just for model-view port

		if (LFootIK)
		{
			UseHumanLimbIK(pInstance,"LftLeg01");
		}
		if (RFootIK)
		{
			UseHumanLimbIK(pInstance,"RgtLeg01");
		}

		if (LArmIK)
		{
			UseHumanLimbIK(pInstance,"LftArm01");
		}
		if (RArmIK)
		{
			UseHumanLimbIK(pInstance,"RgtArm01");
		}

	}
}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

void CModelViewportCE::DrawHeightField()
{
	if (!m_renderer)
		return;

	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );

	// Draw axis.
	if (mv_showBase)
	{
		const float BASE=10.0f;
		pAuxGeom->DrawLine( Vec3(-BASE,0.00f,0.01f),RGBA8(0x7f,0x00,0x00,0x00), Vec3( BASE,0.00f,0.01f),RGBA8(0xff,0x7f,0x7f,0x00) );
		pAuxGeom->DrawLine( Vec3(0.00f,-BASE,0.01f),RGBA8(0x00,0x7f,0x00,0x00), Vec3(0.00f, BASE,0.01f),RGBA8(0x7f,0xff,0x7f,0x00) );
		pAuxGeom->DrawLine( Vec3(0.00f,0.00f,-BASE),RGBA8(0x00,0x00,0x7f,0x00), Vec3(0.00f,0.00f, BASE),RGBA8(0x7f,0x7f,0xff,0x00) );
	}

#define ROW (40*2)
#define COL (40*2)
#define QUADS ((ROW-1)*(COL-1))

	float step = 0.50f;
	const uint32 NumVertices=ROW*COL;
	const uint32 NumIndices=QUADS*2*3;
	uint32 NumVertices2=0;

	m_arrVerticesHF.resize(NumVertices);
	m_arrIndicesHF.resize(NumIndices);

	int32 y0=-COL/2;
	for (uint32 y=0; y<COL; y++)
	{
		int32 x0=-ROW/2;
		for (uint32 x=0; x<ROW; x++)
		{
			Vec3 p=Vec3(step*x0, step*y0,0)+m_GridOrigin;
			m_arrVerticesHF[x+y*ROW]=p;
			NumVertices2++;
			x0++;
		}
		y0++;
	}


	uint32 IndexCounter=0;
	for (uint32 y=0; y<(COL-1); y++)
	{
		for (uint32 x=0; x<(ROW-1); x++)
		{
			uint32 aval = (y+x)&1;
			if (aval)
			{
				m_arrIndicesHF[0+IndexCounter]=x+0+ROW*(y+0); //0
				m_arrIndicesHF[1+IndexCounter]=x+1+ROW*(y+0); //1
				m_arrIndicesHF[2+IndexCounter]=x+0+ROW*(y+1); //3
				IndexCounter+=3;

				m_arrIndicesHF[0+IndexCounter]=x+1+ROW*(y+0); //1
				m_arrIndicesHF[1+IndexCounter]=x+1+ROW*(y+1); //4
				m_arrIndicesHF[2+IndexCounter]=x+0+ROW*(y+1); //3
				IndexCounter+=3;
			}
		}
	}

	//float color1[4] = {1,1,1,1};
	//m_renderer->Draw2dLabel(12,g_ypos,1.2f,color1,false,"heightfield: vb:%d   vb2:%d  ib:%d tri:%d NumIndices:%d",NumVertices,NumVertices2,IndexCounter,IndexCounter/3, NumIndices );
	//g_ypos+=10;

	{
		pAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );
		pAuxGeom->DrawTriangles(&m_arrVerticesHF[0],NumVertices,  &m_arrIndicesHF[0],IndexCounter, RGBA8(0x2f,0x2f,0x3f,0x00) );		
	}

}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

int PostProcessCallback(ICharacterInstance* pInstance,void* pPlayer)
{
	//process bones specific stuff (IK, torso rotation, etc)
	((CModelViewportCE*)pPlayer)->ExternalPostProcessing(pInstance);
	return 1;
}


//----------------------------------------------------------------------------------
//  Lineseg_OBB
//
//	just ONE intersection point is calculated, and thats the entry point           - 
//  Lineseg and OBB are assumed to be in the same space
//
//--- 0x00 = no intersection (output undefined)           --------------------------
//--- 0x01 = intersection (intersection point in output)              --------------
//--- 0x02 = start of Lineseg is inside the OBB (ls.start is output) 
//----------------------------------------------------------------------------------
inline uint8	Lineseg_OBB( const Lineseg& lseg,const Vec3& pos,const OBB& obb, Vec3& output, Vec3& normal ) 
{
	AABB aabb(obb.c-obb.h,obb.c+obb.h);
	Lineseg ls((lseg.start-pos)*obb.m33,(lseg.end-pos)*obb.m33);

	uint8 cflags;
	float cosine;
	Vec3 cut;
	Vec3 lnormal=(ls.start-ls.end).GetNormalized();
	//--------------------------------------------------------------------------------------
	//----         check if "ls.start" is inside of AABB    ---------------------------
	//--------------------------------------------------------------------------------------
	cflags =(ls.start.x>aabb.min.x)<<0;
	cflags|=(ls.start.x<aabb.max.x)<<1;
	cflags|=(ls.start.y>aabb.min.y)<<2;
	cflags|=(ls.start.y<aabb.max.y)<<3;
	cflags|=(ls.start.z>aabb.min.z)<<4;
	cflags|=(ls.start.z<aabb.max.z)<<5;
	if (cflags==0x3f)	{ 
		//ls.start is inside of aabb
		output=obb.m33*ls.start+pos;	return 0x02; 
	}

	//--------------------------------------------------------------------------------------
	//----         check intersection with x-planes           ------------------------------
	//--------------------------------------------------------------------------------------
	if (lnormal.x) {
		if ( (ls.start.x<aabb.min.x) && (ls.end.x>aabb.min.x) ) {
			normal=-obb.m33.GetColumn0();
			cosine = (-ls.start.x+(+aabb.min.x))/lnormal.x;
			cut(aabb.min.x,ls.start.y+(lnormal.y*cosine),ls.start.z+(lnormal.z*cosine));
			//check if cut-point is inside YZ-plane border
			if ((cut.y>aabb.min.y) && (cut.y<aabb.max.y) && (cut.z>aabb.min.z) && (cut.z<aabb.max.z)) {
				output=obb.m33*cut+pos;	return 0x01;
			}
		}
		if ( (ls.start.x>aabb.max.x) && (ls.end.x<aabb.max.x) ) {
			normal=obb.m33.GetColumn0();
			cosine = (+ls.start.x+(-aabb.max.x))/lnormal.x;
			cut(aabb.max.x,ls.start.y-(lnormal.y*cosine),ls.start.z-(lnormal.z*cosine));
			//check if cut-point is inside YZ-plane border
			if ((cut.y>aabb.min.y) && (cut.y<aabb.max.y) && (cut.z>aabb.min.z) && (cut.z<aabb.max.z)) {
				output=obb.m33*cut+pos;	return 0x01;
			}
		}
	}
	//--------------------------------------------------------------------------------------
	//----         check intersection with z-planes           ------------------------------
	//--------------------------------------------------------------------------------------
	if (lnormal.z) {
		if ( (ls.start.z<aabb.min.z) && (ls.end.z>aabb.min.z) ) {
			normal=-obb.m33.GetColumn2();
			cosine = (-ls.start.z+(+aabb.min.z))/lnormal.z;
			cut(ls.start.x+(lnormal.x*cosine),ls.start.y+(lnormal.y*cosine),aabb.min.z);
			//check if cut-point is inside XY-plane border
			if ((cut.x>aabb.min.x) && (cut.x<aabb.max.x) && (cut.y>aabb.min.y) && (cut.y<aabb.max.y)) {
				output=obb.m33*cut+pos;	return 0x01;
			}
		}
		if ( (ls.start.z>aabb.max.z) && (ls.end.z<aabb.max.z) ) {
			normal=obb.m33.GetColumn2();
			cosine = (+ls.start.z+(-aabb.max.z))/lnormal.z;
			cut(ls.start.x-(lnormal.x*cosine),ls.start.y-(lnormal.y*cosine),aabb.max.z);
			//check if cut-point is inside XY-plane border
			if ((cut.x>aabb.min.x) && (cut.x<aabb.max.x) && (cut.y>aabb.min.y) && (cut.y<aabb.max.y)) {
				output=obb.m33*cut+pos;	return 0x01;
			}
		}
	}
	//--------------------------------------------------------------------------------------
	//----         check intersection with y-planes           ------------------------------
	//--------------------------------------------------------------------------------------
	if (lnormal.y) {
		if ( (ls.start.y<aabb.min.y) && (ls.end.y>aabb.min.y) ) {
			normal=-obb.m33.GetColumn1();
			cosine = (-ls.start.y+(+aabb.min.y))/lnormal.y;
			cut(ls.start.x+(lnormal.x*cosine), aabb.min.y, ls.start.z+(lnormal.z*cosine));
			//check if cut-point is inside XZ-plane border
			if ((cut.x>aabb.min.x) && (cut.x<aabb.max.x) && (cut.z>aabb.min.z) && (cut.z<aabb.max.z)) {
				output=obb.m33*cut+pos;	return 0x01;
			}
		}
		if ( (ls.start.y>aabb.max.y) && (ls.end.y<aabb.max.y) ) {
			normal=obb.m33.GetColumn1();
			cosine = (+ls.start.y+(-aabb.max.y))/lnormal.y;
			cut(ls.start.x-(lnormal.x*cosine), aabb.max.y, ls.start.z-(lnormal.z*cosine));
			//check if cut-point is inside XZ-plane border
			if ((cut.x>aabb.min.x) && (cut.x<aabb.max.x) && (cut.z>aabb.min.z) && (cut.z<aabb.max.z)) {
				output=obb.m33*cut+pos;	return 0x01;
			}
		}
	}
	//no intersection
	return 0x00;
}



//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
Plane CModelViewportCE::RayCast(const QuatT& rPhysLocation, f32 fAnimHeight, const Vec3& WorldMiddlePos, const Vec3& obbpos, const OBB& obb, Vec3& Intersection  ) 
{
	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();

	Vec3 WPos = WorldMiddlePos;
	Lineseg ls_middle;
	ls_middle.start	=WPos+Vec3(0.0f,0.0f, 0.6f);
	ls_middle.end		=WPos+Vec3(0.0f,0.0f,-3.0f);
	//pAuxGeom->DrawLine( ls_middle.start,RGBA8(0x1f,0xff,0x1f,0x00),ls_middle.end,RGBA8(0x1f,0xff,0x1f,0x00) );

	Vec3 GroundIntersection(0,0,-9999.0f);
	Vec3 GroundNormal(0,0,1);
	int8 ground = Lineseg_OBB( ls_middle,obbpos,obb, GroundIntersection,GroundNormal );
	GroundNormal				=	GroundNormal*rPhysLocation.q;
	GroundIntersection	=	rPhysLocation.GetInverted()*GroundIntersection;

	if (GroundNormal.z<0.7f)
		GroundNormal.z = 0.7f;
	GroundNormal.Normalize();

	Plane GroundPlane(Vec3(0,0,1),0);
	if (ground==1)
	{
		if ( (GroundIntersection.z) > (fAnimHeight-0.05f) )
			GroundPlane=Plane::CreatePlane(GroundNormal,GroundIntersection);
		else
			GroundPlane=Plane::CreatePlane(Vec3(0,0,1),Vec3(0,0,fAnimHeight));
	}
	//	pIRenderer->D8Print("GroundPlane: %f %f %f  d:%f", GroundPlane.n.x,GroundPlane.n.y,GroundPlane.n.z,GroundPlane.d );

	Intersection = GroundIntersection;
	return GroundPlane;
}




//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

uint32 CModelViewportCE::UseGroundAlignTest(ICharacterInstance* pInstance)
{
	float color1[4] = {1,1,1,1};

	f32 FrameTime = GetIEditor()->GetSystem()->GetITimer()->GetFrameTime();
	//m_AverageFrameTime = pInstance->GetAverageFrameTime(); 
	IRenderAuxGeom* pAuxGeom = m_renderer->GetIRenderAuxGeom();
	ISkeletonAnim* pISkeletonAnim = pInstance->GetISkeletonAnim();
	ISkeletonPose* pISkeletonPose = pInstance->GetISkeletonPose();


	uint32 strg=0;
	if (CheckVirtualKey(VK_LCONTROL))  strg=1;
	if (CheckVirtualKey(VK_RCONTROL))  strg=1;

	/*
	f32 fMoveUpDown=0;
	if (strg==0)
	{
	if (CheckVirtualKey(VK_NUMPAD8)) fMoveUpDown = -1.0f;
	if (CheckVirtualKey(VK_NUMPAD2)) fMoveUpDown = +1.0f;
	}
	m_GroundOBBPos.z += fMoveUpDown*FrameTime;
	*/

	const f32 MaxSlope=0.4f;
	if (strg)
	{
		f32 fRotUpDown=0;
		if ( CheckVirtualKey(VK_NUMPAD8) ) fRotUpDown = +1.0f;
		if ( CheckVirtualKey(VK_NUMPAD2) ) fRotUpDown = -1.0f;
		m_udGround+=fRotUpDown*FrameTime;
		if (m_udGround>+MaxSlope) m_udGround=+MaxSlope;
		if (m_udGround<-MaxSlope) m_udGround=-MaxSlope;

		f32 fRotLeftRight=0;
		if ( CheckVirtualKey(VK_NUMPAD4) ) fRotLeftRight = +1.0f;
		if ( CheckVirtualKey(VK_NUMPAD6) ) fRotLeftRight = -1.0f;
		m_lrGround+=fRotLeftRight*FrameTime;
		if (m_lrGround>+MaxSlope) m_lrGround=+MaxSlope;
		if (m_lrGround<-MaxSlope) m_lrGround=-MaxSlope;

		if ( CheckVirtualKey(VK_NUMPAD5) )
		{
			if (m_udGround<0)
			{
				m_udGround+=FrameTime;
				if (m_udGround>0)	m_udGround=0;
			}
			if (m_udGround>0)
			{
				m_udGround-=FrameTime;
				if (m_udGround<0)	m_udGround=0;
			}

			if (m_lrGround<0)
			{
				m_lrGround+=FrameTime;
				if (m_lrGround>0)	m_lrGround=0;
			}
			if (m_lrGround>0)
			{
				m_lrGround-=FrameTime;
				if (m_lrGround<0)	m_lrGround=0;
			}

		}

	}



	m_GroundOBB.m33	=	Matrix33::CreateRotationXYZ(Ang3(m_udGround,m_lrGround,0));
	pAuxGeom->DrawOBB(m_GroundOBB,m_GroundOBBPos,1,RGBA8(0x1f,0x1f,0x3f,0xff),eBBD_Extremes_Color_Encoded);


	//m_renderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"GroundAngle: %f", acos_tpl(m_GroundOBB.m33.m22) );
	//g_ypos+=14;

	//------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------

	static Plane LSmoothGroundPlane(Vec3(0,0,1),0);
	static Plane LSmoothGroundPlaneRate(Vec3(0,0,1),0);
	{
		int32 LHeelIdx	=	pISkeletonPose->GetJointIDByName("Bip01 L Heel");
		int32 LToeNIdx	=	pISkeletonPose->GetJointIDByName("Bip01 L Toe0");
		if (LHeelIdx<0) 
			return 0;
		if (LToeNIdx<0) 
			return 0;
		Vec3 Final_LHeel	=	m_PhysEntityLocation*pISkeletonPose->GetAbsJointByID(LHeelIdx).t;
		Vec3 Final_LToeN	=	m_PhysEntityLocation*pISkeletonPose->GetAbsJointByID(LToeNIdx).t;

		Vec3 LGroundIntersectionHeel(0,0,-9999.0f);
		Plane LGroundPlaneHeel = RayCast(m_PhysEntityLocation,m_AnimatedCharacter.t.z, Final_LHeel, m_GroundOBBPos, m_GroundOBB, LGroundIntersectionHeel ); 
		//	pAuxGeom->DrawSphere(m_PhysEntityLocation.q*LGroundIntersectionHeel,0.03f,RGBA8(0xff,0xff,0xff,0xff));

		Vec3 LGroundIntersectionToe0(0,0,-9999.0f);
		Plane LGroundPlaneToe0 = RayCast(m_PhysEntityLocation,m_AnimatedCharacter.t.z, Final_LToeN, m_GroundOBBPos, m_GroundOBB, LGroundIntersectionToe0 ); 
		//	pAuxGeom->DrawSphere(m_PhysEntityLocation.q*LGroundIntersectionToe0,0.03f,RGBA8(0xff,0xff,0xff,0xff));


		Vec3 LGroundIntersection=LGroundIntersectionHeel;
		Plane LGroundPlane=LGroundPlaneHeel;
		f32 Ldist = fabsf(LGroundIntersectionHeel.z-LGroundIntersectionToe0.z);
		if (Ldist<0.1f)
		{
			LGroundIntersection	=(LGroundIntersectionHeel+LGroundIntersectionToe0)*0.5f;
			LGroundPlane				=(LGroundPlaneHeel+LGroundPlaneToe0)*0.5f;
		}

		//	m_renderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"LGroundIntersection: %f %f %f",LGroundIntersection.x,LGroundIntersection.y,LGroundIntersection.z );
		//	g_ypos+=14;

		
		SmoothCD( LSmoothGroundPlane, LSmoothGroundPlaneRate, m_AverageFrameTime, LGroundPlane, 0.05f);
		//	m_renderer->Draw2dLabel(12,g_ypos,1.5f,color1,false,"LSmoothGroundPlane: %f %f %f  d:%f", LSmoothGroundPlane.n.x,LSmoothGroundPlane.n.y,LSmoothGroundPlane.n.z,LSmoothGroundPlane.d );
		//	g_ypos+=14;
	}

	static Plane RSmoothGroundPlane(Vec3(0,0,1),0);
	static Plane RSmoothGroundPlaneRate(Vec3(0,0,1),0);

	{
		int32 RHeelIdx	=	pISkeletonPose->GetJointIDByName("Bip01 R Heel");
		int32 RToeNIdx	=	pISkeletonPose->GetJointIDByName("Bip01 R Toe0");
		if (RHeelIdx<0) 
			return 0;
		if (RToeNIdx<0) 
			return 0;
		Vec3 Final_RHeel	=	m_PhysEntityLocation*pISkeletonPose->GetAbsJointByID(RHeelIdx).t;
		Vec3 Final_RToeN	=	m_PhysEntityLocation*pISkeletonPose->GetAbsJointByID(RToeNIdx).t;

		Vec3 RGroundIntersectionHeel(0,0,-9999.0f);
		Plane RGroundPlaneHeel = RayCast(m_PhysEntityLocation,m_AnimatedCharacter.t.z, Final_RHeel, m_GroundOBBPos, m_GroundOBB, RGroundIntersectionHeel ); 
		//	pAuxGeom->DrawSphere(m_PhysEntityLocation.q*RGroundIntersectionHeel,0.03f,RGBA8(0xff,0xff,0xff,0xff));

		Vec3 RGroundIntersectionToe0(0,0,-9999.0f);
		Plane RGroundPlaneToe0 = RayCast(m_PhysEntityLocation,m_AnimatedCharacter.t.z, Final_RToeN, m_GroundOBBPos, m_GroundOBB, RGroundIntersectionToe0 ); 
		//	pAuxGeom->DrawSphere(m_PhysEntityLocation.q*RGroundIntersectionToe0,0.03f,RGBA8(0xff,0xff,0xff,0xff));

		Vec3 RGroundIntersection=RGroundIntersectionHeel;
		Plane RGroundPlane=RGroundPlaneHeel;
		f32 Rdist = fabsf(RGroundIntersectionHeel.z-RGroundIntersectionToe0.z);
		if (Rdist<0.1f)
		{
			RGroundIntersection	=(RGroundIntersectionHeel+RGroundIntersectionToe0)*0.5f;
			RGroundPlane				=(RGroundPlaneHeel+RGroundPlaneToe0)*0.5f;
		}

	
		SmoothCD( RSmoothGroundPlane, RSmoothGroundPlaneRate, m_AverageFrameTime, RGroundPlane, 0.10f);
		//pRenderer->D8Print("SmoothGroundPlane: %f %f %f  d:%f", RSmoothGroundPlane.n.x,RSmoothGroundPlane.n.y,RSmoothGroundPlane.n.z,RSmoothGroundPlane.d );
	}


	//  m_AnimatedCharacter.t.z=MinHigh; //stay on ground
	//	m_AnimatedCharacter.t.z=0.5f; //stay on ground
	pISkeletonPose->SetGroundAlignmentData(false, 0, LSmoothGroundPlane, RSmoothGroundPlane);

	return 1;
}

extern uint32 g_ypos;

//////////////////////////////////////////////////////////////////////////
void CModelViewportCE::DrawCharacter( ICharacterInstance* pInstance, const SRendParams &rRP )
{
	g_ypos = 162;

	__super::DrawCharacter( pInstance, rRP );

	//------------------------------------------------------------------------------
	//---                           unit-tests                                   ---
	//------------------------------------------------------------------------------

	GetISystem()->GetIAnimationSystem()->SetScalingLimits( Vec2(0.7f, 3.5f) );

	IAnimationSet* pIAnimationSet = pInstance->GetIAnimationSet();

	if (m_PlayerControl==0)
		AnimPreview_UnitTest( pInstance, pIAnimationSet, rRP );

	uint32 IsHuman=1;
	if (m_PlayerControl==1 && IsHuman)
		PlayerControl_UnitTest( pInstance, pIAnimationSet, rRP );

	if (m_PlayerControl==2 && IsHuman)
		PathFollowing_UnitTest( pInstance, pIAnimationSet, rRP );

	if (m_PlayerControl==4 && IsHuman)
		Idle2Move_UnitTest( pInstance, pIAnimationSet, rRP );

	if (m_PlayerControl==8 && IsHuman)
		IdleStep_UnitTest( pInstance, pIAnimationSet, rRP );
}

void CModelViewportCE::DrawGrid( const Quat& tmRotation, const Vec3& MotionTranslation,const Vec3& FootSlide, const Matrix33& rGridRot )
{
	__super::DrawGrid( tmRotation,MotionTranslation,FootSlide,rGridRot );
}
