// CharPanel_Animation.cpp : implementation file
//

#include "stdafx.h"
#include "Resource.h"
#include "AnimationInfoLoader.h"
#include <I3DEngine.h>
#include <ICryAnimation.h>
#include "..\ModelViewport.h"
#include "CryCharMorphParams.h"
#include "CharacterEditor.h"
#include "..\Material/MaterialManager.h"
#include "CharPanel_Preset.h"
#include "CharPanel_Animation.h"
#include "..\Clipboard.h"
#define RC_IDM_COPY_NAME	911

/////////////////////////////////////////////////////////////////////////////
// CharPanel_Animation dialog
/////////////////////////////////////////////////////////////////////////////

enum animations_tree_icons
{
	ICON_AIMPOSE,
	ICON_FOLDER,
	ICON_LOOPING,
	ICON_LOOPINGIDLE,
	ICON_NONLOOPING,
	ICON_MISSING,
	ICON_ONDEMAND,
	ICON_LMG,
	ICON_ADDITIVE
};

CharPanel_Animation::CharPanel_Animation( CModelViewport *vp,CWnd* pParent ) : CDialog(CharPanel_Animation::IDD, pParent)
{
	m_ParentWnd = NULL;

	m_ModelViewPort = vp;
	Create(MAKEINTRESOURCE(IDD),pParent);

	m_ParentWnd = static_cast<CCharacterEditor *> (pParent);

	m_DesiredLocomotionSpeed.SetCheck(0);
	m_DesiredLocomotionSpeed.EnableWindow(TRUE);
	m_DesiredTurnSpeed.SetCheck(0);
	m_DesiredTurnSpeed.EnableWindow(TRUE);
	m_LockMoveBody.SetCheck(1);
	m_LockMoveBody.EnableWindow(TRUE);

	m_AnimationDrivenMotion.SetCheck(0);
	m_VerticalMovement.SetCheck(0);

	//animation flags
	m_MirrorAnimation.SetCheck(0);
	m_MirrorAnimation.EnableWindow(TRUE);
	m_LoopAnimation.SetCheck(1);
	m_LoopAnimation.EnableWindow(TRUE);
	m_RepeatLastKey.SetCheck(0);
	m_RepeatLastKey.EnableWindow(TRUE);

	m_VTimeWarping.SetCheck(1);
	m_VTimeWarping.EnableWindow(TRUE);

	m_AdditiveWeight.Create( this, IDC_ADDITIVE_WEIGHT );
	m_AdditiveWeight.SetValue( 0.0f );
	m_AdditiveWeight.EnableWindow(false);
	m_AdditiveWeight.SetRange( -2.0f,2.0f );
	m_AdditiveWeightText.EnableWindow(false);

	m_arrAdditiveWeights[0]=0.0f;
	m_arrAdditiveWeightsSmooth[0]=0.0f;
	m_arrAdditiveWeightsSmoothRate[0]=0.0f;
	for (uint32 i=1; i<16; i++)
	{
		m_arrAdditiveWeights[i]=1.0f;
		m_arrAdditiveWeightsSmooth[i]=1.0f;
		m_arrAdditiveWeightsSmoothRate[i]=1.0f;
	}
	m_DisableMultilayerAnimation.SetCheck(0);
	m_DisableMultilayerAnimation.EnableWindow(TRUE);

	m_AimIKLayer.Create( this, IDC_AIMIK_LAYER );
	m_AimIKLayer.SetValue( 16.0f );
	m_AimIKLayer.EnableWindow(true);
	m_AimIKLayer.SetRange( +1.0f,16.0f );
	m_AimIKLayer.EnableWindow(true);
	m_AimIKLayer.SetInteger(1);

	m_useTransitionTime.SetCheck(0);
	m_useTransitionTime.EnableWindow(TRUE);
	m_TransitionTimeControl.Create( this, IDC_TRANSITION_TIME );
	m_TransitionTimeControl.SetValue( 0.2f );
	m_TransitionTimeControl.EnableWindow(FALSE);


	m_AllowAnimationRestart.SetCheck(0);
	m_AllowAnimationRestart.EnableWindow(TRUE);





	//post processing
	m_FootAnchoring.SetCheck(1);
	m_FootAnchoring.EnableWindow(TRUE);


	m_LArmIK.SetCheck(0);
	m_LArmIK.EnableWindow(TRUE);
	m_RArmIK.SetCheck(0);
	m_RArmIK.EnableWindow(TRUE);
	m_LFootIK.SetCheck(0);
	m_LFootIK.EnableWindow(TRUE);
	m_RFootIK.SetCheck(0);
	m_RFootIK.EnableWindow(TRUE);
	m_GroundAlign.SetCheck(0);
	m_GroundAlign.EnableWindow(TRUE);


	m_PlayControl.SetCheck(0);
	m_PlayControl.EnableWindow(TRUE);
	m_FixedCamera.SetCheck(0);
	m_FixedCamera.EnableWindow(TRUE);

	m_PathFollowing.SetCheck(0);
	m_PathFollowing.EnableWindow(TRUE);
	m_AttachedCamera.SetCheck(0);
	m_AttachedCamera.EnableWindow(TRUE);
	m_Idle2Move.SetCheck(0);
	m_Idle2Move.EnableWindow(TRUE);
	m_IdleStep.SetCheck(0);
	m_IdleStep.EnableWindow(TRUE);


	m_SelectedAnimations=0;
	//m_pCurBone = 0;
	uint32 numSample=0x100;

	m_arrLinearMorphFade.resize(numSample);
	for (uint32 i=0; i<numSample; i++)
		m_arrLinearMorphFade[i]=0.0f;

	m_Gamepad.SetCheck(0);
	m_LinearMorphSequence.SetCheck(0);
	m_UseMorphTargets.SetCheck(1);

	//m_animLoader = 0;
	//m_pBonePreset = 0;

	for(uint32 i=0; i<MAX_PMG_PARAM_DIM; ++i)
	{
		m_ControlParam[i].ShowWindow(FALSE);
		m_ControlParamEdit[i].ShowWindow(FALSE);
		m_ControlParamName[i].ShowWindow(FALSE);
	}
	m_ControlParamName[0].SetWindowText("No PMG Parameters.");
	m_ControlParamName[0].ShowWindow(TRUE);
}

CharPanel_Animation::~CharPanel_Animation()
{
	//if(m_animLoader)
	//delete m_animLoader;
	//m_animLoader = 0;
}



void CharPanel_Animation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILE, m_fileEdit);
	DDX_Control(pDX, IDC_BROWSE_FILE, m_browseFileBtn);
	DDX_Control(pDX, IDC_RELOAD_FILE, m_reloadFileBtn);
	DDX_Control(pDX, IDC_MATERIAL, m_materialBtn);

	DDX_Control(pDX, IDC_CHOOSE_BASE_CHARACTER, m_ChooseBaseCharacter);

	DDX_Control(pDX, IDC_START_SELECTED_ANIMATIONS, m_StartSelectedAnimations);
	DDX_Control(pDX, IDC_SET_AIM_POSES, m_SetAimPoses );
	DDX_Control(pDX, IDC_STOP_ANIMATION_LAYER, m_StopAnimationLayer);

	DDX_Control(pDX, IDC_LAYER, m_layer);
	DDX_Control(pDX, IDC_PRESET, m_preset);
	DDX_Control(pDX, IDC_DESIRED_LOCOMOTION_SPEED, m_DesiredLocomotionSpeed);
	DDX_Control(pDX, IDC_DESIRED_TURN_SPEED, m_DesiredTurnSpeed);
	DDX_Control(pDX, IDC_LOCK_BODYMOVE, m_LockMoveBody);

	//instance flags
	DDX_Control(pDX, IDC_ANIMATION_DRIVEN_MOTION, m_AnimationDrivenMotion);
	DDX_Control(pDX, IDC_VERTICAL_MOVEMENT, m_VerticalMovement);

	//per animation flags
	DDX_Control(pDX, IDC_MIRROR_ANIMATION, m_MirrorAnimation);
	DDX_Control(pDX, IDC_LOOP_ANIMATION, m_LoopAnimation);
	DDX_Control(pDX, IDC_REPEAT_LAST_KEY, m_RepeatLastKey);
	DDX_Control(pDX, IDC_VTIME_WARPING, m_VTimeWarping);
	DDX_Control(pDX, IDC_DISABLE_MULTILAYER_ANIMATION, m_DisableMultilayerAnimation);
	DDX_Control(pDX, IDC_USE_TRANSITION_TIME, m_useTransitionTime);
	DDX_Control(pDX, IDC_ALLOW_ANIMATION_RESTART, m_AllowAnimationRestart);
	DDX_Control(pDX, IDC_ADDITIVE_WEIGHT_TEXT, m_AdditiveWeightText);

	//post processing
	DDX_Control(pDX, IDC_LOOKIK, m_LookIK);
	DDX_Control(pDX, IDC_AIMIK, m_AimIK);

	DDX_Control(pDX, IDC_LFOOTIK, m_LFootIK);
	DDX_Control(pDX, IDC_RFOOTIK, m_RFootIK);
	DDX_Control(pDX, IDC_LARMIK, m_LArmIK);
	DDX_Control(pDX, IDC_RARMIK, m_RArmIK);

	DDX_Control(pDX, IDC_FOOT_ANCHORING, m_FootAnchoring);
	DDX_Control(pDX, IDC_GROUNDALIGN, m_GroundAlign);



	DDX_Control(pDX, IDC_PLAYER_CONTROL, m_PlayControl);
	DDX_Control(pDX, IDC_FIXED_CAMERA, m_FixedCamera);

	DDX_Control(pDX, IDC_PATH_FOLLOWING, m_PathFollowing);
	DDX_Control(pDX, IDC_ATTACHED_CAM, m_AttachedCamera);

	DDX_Control(pDX, IDC_IDLE2MOVE, m_Idle2Move);
	DDX_Control(pDX, IDC_IDLESTEP, m_IdleStep);

	DDX_Control(pDX, IDC_USE_GAMEPAD, m_Gamepad);
	DDX_Control(pDX, IDC_LINEAR_MORPH_SEQUENCE, m_LinearMorphSequence);
	DDX_Control(pDX, IDC_USE_MORPHTARGETS, m_UseMorphTargets);

	DDX_Control(pDX, IDC_PMG_PARAM_NAME_1, m_ControlParamName[0]);
	DDX_Control(pDX, IDC_PMG_PARAM_NAME_2, m_ControlParamName[1]);
	DDX_Control(pDX, IDC_PMG_PARAM_NAME_3, m_ControlParamName[2]);
	DDX_Control(pDX, IDC_PMG_PARAM_NAME_4, m_ControlParamName[3]);
	DDX_Control(pDX, IDC_PMG_PARAM_NAME_5, m_ControlParamName[4]);

}


BEGIN_MESSAGE_MAP(CharPanel_Animation, CDialog)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_BROWSE_FILE, OnBnClickedBrowseFile)
	ON_BN_CLICKED(IDC_RELOAD_FILE, OnBnClickedReloadFile)
	ON_BN_CLICKED(IDC_ANIMATE_LIGHTS, OnAnimateLights)
	ON_BN_CLICKED(IDC_MATERIAL, OnBnClickedMaterial)

	ON_BN_CLICKED(IDC_CHOOSE_BASE_CHARACTER, OnButton_ChooseBaseCharacter)

	ON_CONTROL(WMU_FS_CHANGED, IDC_BLENDSPACESLIDER_X, OnSlider_BlendSpaceX)
	ON_EN_CHANGE(IDC_BLENDSPACENUMBER_X, OnNumber_BlendSpaceX)
	ON_CONTROL(WMU_FS_CHANGED, IDC_BLENDSPACESLIDER_Y, OnSlider_BlendSpaceY)
	ON_EN_CHANGE(IDC_BLENDSPACENUMBER_Y, OnNumber_BlendSpaceY)
	ON_CONTROL(WMU_FS_CHANGED, IDC_BLENDSPACESLIDER_Z, OnSlider_BlendSpaceZ)
	ON_EN_CHANGE(IDC_BLENDSPACENUMBER_Z, OnNumber_BlendSpaceZ)



	ON_CONTROL(WMU_FS_CHANGED, IDC_CONTROL_PARAM_1, OnSlider_ControlParam1)
	ON_EN_CHANGE(IDC_CONTROL_PARAM_NUM_1, OnNumber_ControlParam1)
	ON_CONTROL(WMU_FS_CHANGED, IDC_CONTROL_PARAM_2, OnSlider_ControlParam2)
	ON_EN_CHANGE(IDC_CONTROL_PARAM_NUM_2, OnNumber_ControlParam2)
	ON_CONTROL(WMU_FS_CHANGED, IDC_CONTROL_PARAM_3, OnSlider_ControlParam3)
	ON_EN_CHANGE(IDC_CONTROL_PARAM_NUM_3, OnNumber_ControlParam3)
	ON_CONTROL(WMU_FS_CHANGED, IDC_CONTROL_PARAM_4, OnSlider_ControlParam4)
	ON_EN_CHANGE(IDC_CONTROL_PARAM_NUM_4, OnNumber_ControlParam4)
	ON_CONTROL(WMU_FS_CHANGED, IDC_CONTROL_PARAM_5, OnSlider_ControlParam5)
	ON_EN_CHANGE(IDC_CONTROL_PARAM_NUM_5, OnNumber_ControlParam5)

	ON_CONTROL(WMU_FS_CHANGED, IDC_POLARX_SLIDER, OnSlider_PolarX)
	ON_EN_CHANGE(IDC_POLARX_NUMBER, OnNumber_PolarX)
	ON_CONTROL(WMU_FS_CHANGED, IDC_POLARY_SLIDER, OnSlider_PolarY)
	ON_EN_CHANGE(IDC_POLARY_NUMBER, OnNumber_PolarY)

	ON_BN_CLICKED(IDC_ANIMATION_DRIVEN_MOTION, OnButton_TransRot2000)
	ON_BN_CLICKED(IDC_VERTICAL_MOVEMENT, OnButton_VerticalMovement)

	ON_BN_CLICKED(IDC_LOOP, OnLoop)
	ON_BN_CLICKED(IDC_VTIME_WARPING, OnButton_VTimeWarping)
	ON_BN_CLICKED(IDC_USE_TRANSITION_TIME, OnButton_GetTransitionTime)
	ON_EN_CHANGE(IDC_ADDITIVE_WEIGHT, OnNumber_GetAdditiveWeightValue)

	ON_BN_CLICKED(IDC_FOOT_ANCHORING, OnButton_FootAnchoring)

	ON_BN_CLICKED(IDC_LFOOTIK, OnButton_LFootIK)
	ON_BN_CLICKED(IDC_RFOOTIK, OnButton_RFootIK)
	ON_BN_CLICKED(IDC_LARMIK, OnButton_LArmIK)
	ON_BN_CLICKED(IDC_RARMIK, OnButton_RArmIK)
	ON_BN_CLICKED(IDC_GROUNDALIGN, OnButton_GroundAlign)


	ON_BN_CLICKED(IDC_START_SELECTED_ANIMATIONS, OnButton_StartSelectedAnimations)
	ON_BN_CLICKED(IDC_SET_AIM_POSES, OnButton_SetAimPoses)

	ON_BN_CLICKED(IDC_STOP_ANIMATION_LAYER, OnButton_StopAnimationInLayer)
	ON_BN_CLICKED(IDC_RESETANIMATIONS, OnButton_StopAnimationsAll)
	ON_BN_CLICKED(IDC_RESET_MODEL, OnButton_ResetModel)


	ON_BN_CLICKED(IDC_PLAYER_CONTROL, OnButton_PlayerControl)
	ON_BN_CLICKED(IDC_PATH_FOLLOWING, OnButton_PathFollowing)
	ON_BN_CLICKED(IDC_IDLE2MOVE, OnButton_Idle2Move)
	ON_BN_CLICKED(IDC_IDLESTEP, OnButton_IdleStep)


	ON_CONTROL(WMU_FS_CHANGED, IDC_ANIMATION_SPEED_SLIDER, OnSlider_AnimationSpeed)
	ON_EN_CHANGE(IDC_ANIMATION_SPEED_NUMBER, OnNumber_AnimationSpeed)

	ON_BN_CLICKED(IDC_RECALCULATE, OnBnClickedRecalculate)
	ON_BN_CLICKED(IDC_LINEAR_MORPH_SEQUENCE, OnButton_LinearMorphSequence)

	ON_CBN_SELCHANGE(IDC_PRESET, OnPresetSelect)
	ON_CBN_SELCHANGE(IDC_LAYER, OnChange_GetLayer)

END_MESSAGE_MAP()


BOOL CharPanel_Animation::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_layer.SetCurSel(0);


	m_AnimationDrivenMotion.SetCheck(BST_CHECKED);
	m_VerticalMovement.SetCheck(BST_CHECKED);

	//--------------------------------------------------------------------
	m_BlendSpaceSliderX.SubclassDlgItem( IDC_BLENDSPACESLIDER_X,this );
	m_BlendSpaceSliderX.SetRangeFloat( -1.0f,+1.0f  );
	m_BlendSpaceSliderX.SetValue(0.0f);
	m_BlendSpaceSliderX.EnableWindow(TRUE);

	m_BlendSpaceNumberX.Create( this,IDC_BLENDSPACENUMBER_X );
	m_BlendSpaceNumberX.SetRange( -1.0f,+10.0f );
	m_BlendSpaceNumberX.SetInternalPrecision( 3 );
	m_BlendSpaceNumberX.SetValue(0.0f);
	m_BlendSpaceNumberX.EnableWindow(TRUE);

	//----------------------------------------------------------------------------
	m_BlendSpaceSliderY.SubclassDlgItem( IDC_BLENDSPACESLIDER_Y,this );
	m_BlendSpaceSliderY.SetRangeFloat( -1.0f,+1.0f  );
	m_BlendSpaceSliderY.SetValue(0.0f);
	m_BlendSpaceSliderY.EnableWindow(TRUE);

	m_BlendSpaceNumberY.Create( this,IDC_BLENDSPACENUMBER_Y );
	m_BlendSpaceNumberY.SetRange( -10.0f,+10.0f );
	m_BlendSpaceNumberY.SetInternalPrecision( 3 );
	m_BlendSpaceNumberY.SetValue(0.0f);
	m_BlendSpaceNumberY.EnableWindow(TRUE);

	//----------------------------------------------------------------------------
	m_BlendSpaceSliderZ.SubclassDlgItem( IDC_BLENDSPACESLIDER_Z,this );
	m_BlendSpaceSliderZ.SetRangeFloat( -1.0f,+1.0f  );
	m_BlendSpaceSliderZ.SetValue(0.0f);
	m_BlendSpaceSliderZ.EnableWindow(TRUE);

	m_BlendSpaceNumberZ.Create( this,IDC_BLENDSPACENUMBER_Z );
	m_BlendSpaceNumberZ.SetRange( -1.0f,+1.0f );
	m_BlendSpaceNumberZ.SetInternalPrecision( 3 );
	m_BlendSpaceNumberZ.SetValue(0.0f);
	m_BlendSpaceNumberZ.EnableWindow(TRUE);



	//--------------------------------------------------------------------
	m_PolarX_Slider.SubclassDlgItem( IDC_POLARX_SLIDER,this );
	m_PolarX_Slider.SetRangeFloat( -2.55f,+2.55f  );
	m_PolarX_Slider.SetValue(0.0f);
	m_PolarX_Slider.EnableWindow(TRUE);

	m_PolarX_Number.Create( this,IDC_POLARX_NUMBER );
	m_PolarX_Number.SetRange( -2.55f,+2.55f );
	m_PolarX_Number.SetInternalPrecision( 3 );
	m_PolarX_Number.SetValue(0.0f);
	m_PolarX_Number.EnableWindow(TRUE);

	//----------------------------------------------------------------------------
	m_PolarY_Slider.SubclassDlgItem( IDC_POLARY_SLIDER,this );
	m_PolarY_Slider.SetRangeFloat( -1.55f,+1.55f  );
	m_PolarY_Slider.SetValue(0.0f);
	m_PolarY_Slider.EnableWindow(TRUE);

	m_PolarY_Number.Create( this,IDC_POLARY_NUMBER );
	m_PolarY_Number.SetRange( -1.55f,+1.55f );
	m_PolarY_Number.SetInternalPrecision( 3 );
	m_PolarY_Number.SetValue(0.0f);
	m_PolarY_Number.EnableWindow(TRUE);

	//------------------------------------------------------------------------------
	// PGM test control
	//------------------------------------------------------------------------------
	static f32 minV1 = -10.0f, maxV1 = 10.0f;
	static f32 minV2 = -10.0f, maxV2 = 10.0f;
	static f32 minV3 = -10.0f, maxV3 = 10.0f;
	static f32 minV4 = -10.0f, maxV4 = 10.0f;
	static f32 minV5 = -10.0f, maxV5 = 10.0f;
	//--------------------------------------------------------------------
	m_ControlParam[0].SubclassDlgItem( IDC_CONTROL_PARAM_1, this );
	m_ControlParam[0].SetRangeFloat( minV1, maxV1  );
	m_ControlParam[0].SetValue(3.0f);
	m_ControlParam[0].EnableWindow(FALSE);

	m_ControlParamEdit[0].Create( this,IDC_CONTROL_PARAM_NUM_1 );
	m_ControlParamEdit[0].SetRange( minV1, maxV1  );
	m_ControlParamEdit[0].SetInternalPrecision( 3 );
	m_ControlParamEdit[0].SetValue(3.0f);
	m_ControlParamEdit[0].EnableWindow(FALSE);

	//--------------------------------------------------------------------
	m_ControlParam[1].SubclassDlgItem( IDC_CONTROL_PARAM_2, this );
	m_ControlParam[1].SetRangeFloat( minV2, maxV2  );

	m_ControlParam[1].SetValue(0.0f);
	m_ControlParam[1].EnableWindow(FALSE);

	m_ControlParamEdit[1].Create( this,IDC_CONTROL_PARAM_NUM_2 );
	m_ControlParamEdit[1].SetRange( minV2, maxV2  );
	m_ControlParamEdit[1].SetInternalPrecision( 3 );
	m_ControlParamEdit[1].SetValue(0.0f);
	m_ControlParamEdit[1].EnableWindow(FALSE);

	//--------------------------------------------------------------------
	m_ControlParam[2].SubclassDlgItem( IDC_CONTROL_PARAM_3, this );
	m_ControlParam[2].SetRangeFloat( minV3, maxV3  );

	m_ControlParam[2].SetValue(0.0f);
	m_ControlParam[2].EnableWindow(FALSE);

	m_ControlParamEdit[2].Create( this,IDC_CONTROL_PARAM_NUM_3 );
	m_ControlParamEdit[2].SetRange( minV3, maxV3  );
	m_ControlParamEdit[2].SetInternalPrecision( 3 );
	m_ControlParamEdit[2].SetValue(0.0f);
	m_ControlParamEdit[2].EnableWindow(FALSE);

	//--------------------------------------------------------------------
	m_ControlParam[3].SubclassDlgItem( IDC_CONTROL_PARAM_4, this );
	m_ControlParam[3].SetRangeFloat( minV4, maxV4  );

	m_ControlParam[3].SetValue(0.0f);
	m_ControlParam[3].EnableWindow(FALSE);

	m_ControlParamEdit[3].Create( this,IDC_CONTROL_PARAM_NUM_4 );
	m_ControlParamEdit[3].SetRange( minV4, maxV4  );
	m_ControlParamEdit[3].SetInternalPrecision( 3 );
	m_ControlParamEdit[3].SetValue(0.0f);
	m_ControlParamEdit[3].EnableWindow(FALSE);

	//--------------------------------------------------------------------
	m_ControlParam[4].SubclassDlgItem( IDC_CONTROL_PARAM_5, this );
	m_ControlParam[4].SetRangeFloat( minV5, maxV5  );

	m_ControlParam[4].SetValue(0.0f);
	m_ControlParam[4].EnableWindow(FALSE);

	m_ControlParamEdit[4].Create( this,IDC_CONTROL_PARAM_NUM_5 );
	m_ControlParamEdit[4].SetRange( minV5, maxV5  );
	m_ControlParamEdit[4].SetInternalPrecision( 3 );
	m_ControlParamEdit[4].SetValue(0.0f);
	m_ControlParamEdit[4].EnableWindow(FALSE);

	//--------------------------------------------------------------------

	m_Gamepad.SetCheck(0);
	m_LinearMorphSequence.SetCheck(0);

	m_LinearMorphSlider.SubclassDlgItem( IDC_LINEAR_MORPH_SLIDER,this );
	m_LinearMorphSlider.SetRangeFloat( -0.0f,+1.0f  );
	m_LinearMorphSlider.SetValue(0.5f);
	m_LinearMorphSlider.EnableWindow(FALSE);

	//m_LayerMotionInterpolationNumber.Create( this,IDC_LINEAR_MORPH_NUMBER );
	//m_LayerMotionInterpolationNumber.SetRange( 0.0f,+0.999f );
	//m_LayerMotionInterpolationNumber.SetInternalPrecision( 3 );
	//m_LayerMotionInterpolationNumber.SetValue(0.5f);
	//m_LayerMotionInterpolationNumber.EnableWindow(TRUE);

	//--------------------------------------------------------------------

	m_AnimationSpeedSlider.SubclassDlgItem( IDC_ANIMATION_SPEED_SLIDER,this );
	m_AnimationSpeedSlider.SetRangeFloat( -1,2 );
	m_AnimationSpeedSlider.SetValue(1);
	m_AnimationSpeedSlider.EnableWindow(TRUE);

	m_AnimationSpeedNumber.Create( this,IDC_ANIMATION_SPEED );
	m_AnimationSpeedNumber.SetRange( -1,2 );
	m_AnimationSpeedNumber.SetInternalPrecision( 3 );
	m_AnimationSpeedNumber.SetValue(1);
	m_AnimationSpeedNumber.EnableWindow(TRUE);

	//--------------------------------------------------------------------

	m_preset.ResetContent();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CharPanel_Animation::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// Route the commands to the MainFrame
	if (AfxGetMainWnd())
		AfxGetMainWnd()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}



/////////////////////////////////////////////////////////////////////////////
// CharPanel_Animation message handlers

CString CharPanel_Animation::GetCurrFileName()
{
	CString filename;
	m_fileEdit.GetWindowText( filename );
	return filename;
}

void CharPanel_Animation::ResetPMGControls()
{
	for(uint32 i=0; i<MAX_PMG_PARAM_DIM; ++i)
	{
		m_ControlParam[i].SetValue(0.0f);
		m_ControlParamEdit[i].SetValue(0.0f);
	}
};

CString CharPanel_Animation::GetCurrAnimName(int nNum)
{
	std::vector<CString> names;
	if(m_ParentWnd)
		m_ParentWnd->GetAnimationPanel()->GetSelectedAnimations(names);

	if(!names.empty())
		return names[0];

	return NULL;
}

void CharPanel_Animation::OnButton_StartSelectedAnimations()
{
	m_ModelViewPort->SetPaused(false);
	m_ModelViewPort->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
}

//string zsAimName;
void CharPanel_Animation::OnButton_SetAimPoses()
{
	/*	uint32 nCount = g_SortedItems.size();
	m_SelectedAnimations=nCount;
	if (nCount)
	{
	uint32 IDs = GetAnimIDs();
	int16 id0=(int16)(IDs & 0xffff);
	if (id0>=0)
	{
	zsAimName = GetAnimNameByID(id0);
	const char* zsName = (const char*)zsAimName;
	m_ModelViewPort->SetAimPose(zsName);
	}
	}*/
}


void CharPanel_Animation::OnSlider_ControlParam1()
{
	f32 val = m_ControlParam[0].GetValue();
	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	m_ControlParamEdit[0].SetValue( val );

	CString paramName;
	m_ControlParamName[0].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	pISkeletonAnim->SetControlParam(val, sName.c_str(), 0);
}

void CharPanel_Animation::OnSlider_ControlParam2()
{
	f32 val = m_ControlParam[1].GetValue();
	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	m_ControlParamEdit[1].SetValue( val );

	CString paramName;
	m_ControlParamName[1].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	pISkeletonAnim->SetControlParam(val, sName.c_str(), 1);
}

void CharPanel_Animation::OnSlider_ControlParam3()
{
	f32 val = m_ControlParam[2].GetValue();
	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	m_ControlParamEdit[2].SetValue( val );

	CString paramName;
	m_ControlParamName[2].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	pISkeletonAnim->SetControlParam(val, sName.c_str(), 2);
}

void CharPanel_Animation::OnSlider_ControlParam4()
{
	f32 val = m_ControlParam[3].GetValue();
	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	m_ControlParamEdit[3].SetValue( val );

	CString paramName;
	m_ControlParamName[3].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	pISkeletonAnim->SetControlParam(val, sName.c_str(), 3);
}

void CharPanel_Animation::OnSlider_ControlParam5()
{
	f32 val = m_ControlParam[4].GetValue();
	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	m_ControlParamEdit[4].SetValue( val );

	CString paramName;
	m_ControlParamName[4].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	pISkeletonAnim->SetControlParam(val, sName.c_str(), 4);
}


void CharPanel_Animation::OnNumber_ControlParam1()
{
	float val = m_ControlParamEdit[0].GetValue();
	m_ControlParam[0].SetValue( val );

	CString paramName;
	m_ControlParamName[0].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	if(pISkeletonAnim)
		pISkeletonAnim->SetControlParam(val, sName.c_str(), 0);


}

void CharPanel_Animation::OnNumber_ControlParam2()
{
	float val = m_ControlParamEdit[1].GetValue();
	m_ControlParam[1].SetValue( val );

	CString paramName;
	m_ControlParamName[1].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	if(pISkeletonAnim)
		pISkeletonAnim->SetControlParam(val, sName.c_str(), 1);
}

void CharPanel_Animation::OnNumber_ControlParam3()
{
	float val = m_ControlParamEdit[2].GetValue();
	m_ControlParam[2].SetValue( val );

	CString paramName;
	m_ControlParamName[2].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	if(pISkeletonAnim)
		pISkeletonAnim->SetControlParam(val, sName.c_str(), 2);
}

void CharPanel_Animation::OnNumber_ControlParam4()
{
	float val = m_ControlParamEdit[3].GetValue();
	m_ControlParam[3].SetValue( val );

	CString paramName;
	m_ControlParamName[3].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	if(pISkeletonAnim)
		pISkeletonAnim->SetControlParam(val, sName.c_str(), 3);
}

void CharPanel_Animation::OnNumber_ControlParam5()
{
	float val = m_ControlParamEdit[4].GetValue();
	m_ControlParam[4].SetValue( val );

	CString paramName;
	m_ControlParamName[4].GetWindowText(paramName);
	string sName = string(paramName.GetBuffer());
	m_prePMGParams[sName] = val;

	ISkeletonAnim* pISkeletonAnim = m_ModelViewPort->GetCharacterAnim()->GetISkeletonAnim();
	if(pISkeletonAnim)
		pISkeletonAnim->SetControlParam(val, sName.c_str(), 4);
}





void CharPanel_Animation::OnSlider_BlendSpaceX()
{
	f32 val = m_BlendSpaceSliderX.GetValue();

	uint32 ForceSpeed=GetDesiredLocomotionSpeed();
	if (ForceSpeed)
		val=(val+1)*5;

	m_BlendSpaceNumberX.SetValue( val );
}
void CharPanel_Animation::OnNumber_BlendSpaceX()
{
	float val = m_BlendSpaceNumberX.GetValue();
	m_BlendSpaceSliderX.SetValue( val );
}
f32 CharPanel_Animation::GetBlendSpaceFlexX(f32 sec, f32 aft) 
{ 
	static f32 SmoothBlendSpaceX=0;
	static f32 SmoothBlendSpaceXRate=0;
	SmoothCD( SmoothBlendSpaceX,SmoothBlendSpaceXRate, aft, m_BlendSpaceSliderX.GetValue(), 0.40f);
	return SmoothBlendSpaceX;	
}



void CharPanel_Animation::OnSlider_BlendSpaceY()
{
	f32 val = m_BlendSpaceSliderY.GetValue();

	uint32 ForceSpeed=GetDesiredTurnSpeed();
// 	if (ForceSpeed)
// 		val=(val*gf_PI);

	m_BlendSpaceNumberY.SetValue( val );
}
void CharPanel_Animation::OnNumber_BlendSpaceY()
{
	float val = m_BlendSpaceNumberY.GetValue();
	m_BlendSpaceSliderY.SetValue( val );
}
f32 CharPanel_Animation::GetBlendSpaceFlexY(f32 sec, f32 aft) 
{ 
	static f32 SmoothBlendSpaceY=0;
	static f32 SmoothBlendSpaceYRate=0;
	SmoothCD( SmoothBlendSpaceY,SmoothBlendSpaceYRate, aft, m_BlendSpaceSliderY.GetValue(), 0.40f);
	return SmoothBlendSpaceY;	
}



void CharPanel_Animation::OnSlider_BlendSpaceZ()
{
	f32 val = m_BlendSpaceSliderZ.GetValue();
	m_BlendSpaceNumberZ.SetValue( val );
}
void CharPanel_Animation::OnNumber_BlendSpaceZ()
{
	float val = m_BlendSpaceNumberZ.GetValue();
	m_BlendSpaceSliderZ.SetValue( val );
}
f32 CharPanel_Animation::GetBlendSpaceFlexZ(f32 sec, f32 aft) 
{ 
	static f32 SmoothBlendSpaceZ=0;
	static f32 SmoothBlendSpaceZRate=0;
	SmoothCD( SmoothBlendSpaceZ,SmoothBlendSpaceZRate, aft, m_BlendSpaceSliderZ.GetValue(), 0.40f);
	return SmoothBlendSpaceZ;	
}






void CharPanel_Animation::OnSlider_PolarX()
{
	f32 valx = m_PolarX_Slider.GetValue();
	f32 valy = m_PolarY_Slider.GetValue();
	m_PolarX_Number.SetValue( valx);
	ICharacterInstance* pICharInstance =	m_ModelViewPort->GetCharacterBase();
	if (pICharInstance)
	{
		ISkeletonPose* pISkeletonPose = pICharInstance->GetISkeletonPose();
		pISkeletonPose->SetAimIKPolarCoordinates( Vec2(valx,valy) );
	}
}
void CharPanel_Animation::OnNumber_PolarX()
{
	float val = m_PolarX_Number.GetValue();
	m_PolarX_Slider.SetValue( val );
}


void CharPanel_Animation::OnSlider_PolarY()
{
	f32 valx = m_PolarX_Slider.GetValue();
	f32 valy = m_PolarY_Slider.GetValue();
	m_PolarY_Number.SetValue( valy );
	ICharacterInstance* pICharInstance =	m_ModelViewPort->GetCharacterBase();
	if (pICharInstance)
	{
		ISkeletonPose* pISkeletonPose = pICharInstance->GetISkeletonPose();
		pISkeletonPose->SetAimIKPolarCoordinates( Vec2(valx,valy) );
	}
}
void CharPanel_Animation::OnNumber_PolarY()
{
	float val = m_PolarY_Number.GetValue();
	m_PolarY_Slider.SetValue( val );
}








void CharPanel_Animation::OnSlider_LinearMorph()
{
	f32 val = m_LinearMorphSlider.GetValue();
	//m_LayerMotionInterpolationNumber.SetValue( val );
}

f32 CharPanel_Animation::GetLinearMorphSliderFlex(f32 sec, f32 aft) 
{ 
	uint32 numValues=m_arrLinearMorphFade.size();
	for (int32 i=(numValues-2); i>-1; i--)
		m_arrLinearMorphFade[i+1] = m_arrLinearMorphFade[i];
	m_arrLinearMorphFade[0] = m_LinearMorphSlider.GetValue();

	f32 val=0;

	if (aft==0.0f)
		aft=0.000001f;

	uint32 avrg_frames = sec / aft;  //get amount frames of the last x.xx seconds
	if (avrg_frames>numValues)	avrg_frames=numValues;
	if (avrg_frames<1)	avrg_frames=1;

	for (uint32 i=0; i<avrg_frames; i++)
		val += m_arrLinearMorphFade[i];
	val /= avrg_frames;

	return val;	
}



bool CharPanel_Animation::GetAnimLightState()
{
	return IsDlgButtonChecked( IDC_ANIMATE_LIGHTS ) == TRUE;
}

void CharPanel_Animation::OnAnimateLights() 
{
	// TODO: Add your control notification handler code here
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(IDC_ANIMATE_LIGHTS,0),(LPARAM)GetSafeHwnd() );
}






void CharPanel_Animation::OnSlider_AnimationSpeed()
{
	float val = m_AnimationSpeedSlider.GetValue();
	m_AnimationSpeedNumber.SetValue( val );
	ICharacterInstance* pCharacter = m_ModelViewPort->GetCharacterBase();
	if (pCharacter)
		pCharacter->SetAnimationSpeed(val);
}
void CharPanel_Animation::OnNumber_AnimationSpeed()
{
	float val = m_AnimationSpeedNumber.GetValue();
	m_AnimationSpeedSlider.SetValue( val );
	ICharacterInstance* pCharacter = m_ModelViewPort->GetCharacterBase();
	if (pCharacter)
		pCharacter->SetAnimationSpeed(val);
}


void CharPanel_Animation::OnLoop() 
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
}




void CharPanel_Animation::OnButton_StopAnimationInLayer()
{
	int nLayer = GetLayer();
	m_ModelViewPort->StopAnimationInLayer(nLayer);
}

void CharPanel_Animation::OnButton_StopAnimationsAll()
{
	m_ModelViewPort->GetCharacterBase()->GetISkeletonAnim()->StopAnimationsAllLayers();
	m_ModelViewPort->SetPaused(true);
}
void CharPanel_Animation::OnButton_ResetModel()
{
	m_ModelViewPort->GetCharacterBase()->GetISkeletonPose()->SetDefaultPose();
}

void CharPanel_Animation::OnButton_ChooseBaseCharacter()
{
	m_ModelViewPort->m_pCharacterAnim = m_ModelViewPort->GetCharacterBase();
	m_ModelViewPort->UpdateAnimationList();

	if(m_ParentWnd)
		m_ParentWnd->GetAnimationPanel()->UpdateAnimations(m_ModelViewPort->GetCharacterBase());

}

void CharPanel_Animation::SetBaseCharacter()
{
	m_ModelViewPort->m_pCharacterAnim = m_ModelViewPort->GetCharacterBase();
	m_ModelViewPort->UpdateAnimationList();
}

//////////////////////////////////////////////////////////////////////////
void CharPanel_Animation::OnBnClickedBrowseFile()
{
	CString file;
	m_fileEdit.GetWindowText( file );
	if (CFileUtil::SelectSingleFile( EFILE_TYPE_GEOMETRY,file ))
	{
		m_ModelViewPort->LoadObject( file,1 );
		m_fileEdit.SetWindowText( file );
	}
}

//////////////////////////////////////////////////////////////////////////
void CharPanel_Animation::SetFileName( const CString &filename )
{
	m_fileEdit.SetWindowText( filename );
}

//////////////////////////////////////////////////////////////////////////
void CharPanel_Animation::OnBnClickedReloadFile()
{
	CString filename;
	m_fileEdit.GetWindowText( filename );
	m_ModelViewPort->LoadObject( filename,1 );
}

//////////////////////////////////////////////////////////////////////////
void CharPanel_Animation::OnBnClickedMaterial()
{
	// Try to find the material used by the displayed asset.
	IMaterial* pMaterial = 0;
	if (m_ModelViewPort->GetCharacterBase() && m_ModelViewPort->GetCharacterBase()->GetMaterial())
		pMaterial = m_ModelViewPort->GetCharacterBase()->GetMaterial();
	else if (m_ModelViewPort->GetStaticObject() && m_ModelViewPort->GetStaticObject()->GetMaterial())
		pMaterial = m_ModelViewPort->GetStaticObject()->GetMaterial();

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

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
void CharPanel_Animation::OnButton_TransRot2000()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	uint32 TransRot2000Status = m_AnimationDrivenMotion.GetState() & BST_CHECKED;
}

void CharPanel_Animation::OnButton_VerticalMovement()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	uint32 VerticalMovement = m_VerticalMovement.GetState() & BST_CHECKED;
}

void CharPanel_Animation::OnButton_VTimeWarping()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	uint32 TimeWarpingStatus = m_VTimeWarping.GetState() & BST_CHECKED;
}


void CharPanel_Animation::OnButton_FootAnchoring()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	//uint32 FVEStatus = m_FootAnchoring.GetState() & BST_CHECKED;
	//m_FootIK.EnableWindow(FVEStatus);
}


void CharPanel_Animation::OnButton_LFootIK()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	uint32 buttons = m_LFootIK.GetState() & BST_CHECKED;
	if (buttons)
	{
		//	m_LFootIK.SetCheck(1);
		m_RFootIK.SetCheck(0);
		m_LArmIK.SetCheck(0);
		m_RArmIK.SetCheck(0);
		m_GroundAlign.SetCheck(0);
	}
}

void CharPanel_Animation::OnButton_RFootIK()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	uint32 button = m_RFootIK.GetState() & BST_CHECKED;
	if (button)
	{
		m_LFootIK.SetCheck(0);
		//	m_RFootIK.SetCheck(1);
		m_LArmIK.SetCheck(0);
		m_RArmIK.SetCheck(0);
		m_GroundAlign.SetCheck(0);
	}
}

void CharPanel_Animation::OnButton_LArmIK()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	uint32 button = m_LArmIK.GetState() & BST_CHECKED;
	if (button)
	{
		m_LFootIK.SetCheck(0);
		m_RFootIK.SetCheck(0);
		//	m_LArmIK.SetCheck(1);
		m_RArmIK.SetCheck(0);
		m_GroundAlign.SetCheck(0);
	}
}

void CharPanel_Animation::OnButton_RArmIK()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	uint32 button = m_RArmIK.GetState() & BST_CHECKED;
	if (button)
	{
		m_LFootIK.SetCheck(0);
		m_RFootIK.SetCheck(0);
		m_LArmIK.SetCheck(0);
		//	m_RArmIK.SetCheck(1);
		m_GroundAlign.SetCheck(0);
	}
}

void CharPanel_Animation::OnButton_GroundAlign()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	uint32 button = m_GroundAlign.GetState() & BST_CHECKED;
	if (button)
	{
		m_LFootIK.SetCheck(0);
		m_RFootIK.SetCheck(0);
		m_LArmIK.SetCheck(0);
		m_RArmIK.SetCheck(0);
		//m_GroundAlign.SetCheck(1);
	}
}


void CharPanel_Animation::OnButton_PlayerControl()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );

	m_ModelViewPort->SetPlayerPos(Vec3(ZERO));
	uint32 PlayerControlStatus = m_PlayControl.GetState() & BST_CHECKED;

	ICharacterInstance* pCharacter = m_ModelViewPort->GetCharacterBase();
	if (pCharacter)
	{
		if (PlayerControlStatus)
		{
			//enable player-control
			pCharacter->GetISkeletonPose()->SetDefaultPose();
			m_ModelViewPort->SetPlayerControl(PlayerControlStatus);
		}
		else
		{
			//normal character viewer
			m_ModelViewPort->SetPlayerControl(0);
			pCharacter->GetISkeletonPose()->SetDefaultPose();
			Vec3 VPos=Vec3(0,3,2);
			Vec3 VTar=Vec3(0,0,1);
			Matrix33 orient; orient.SetRotationVDir( (VTar-VPos).GetNormalized() );
			m_ModelViewPort->m_Camera.SetMatrix( Matrix34(orient,VPos)  );
		}
	}
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
void CharPanel_Animation::OnButton_PathFollowing()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );

	m_ModelViewPort->SetPlayerPos(Vec3(ZERO));
	uint32 PathFollowingStatus = m_PathFollowing.GetState() & BST_CHECKED;


	ICharacterInstance* pCharacter = m_ModelViewPort->GetCharacterBase();
	if (pCharacter)
	{
		if (PathFollowingStatus)
		{
			//enable path-following
			pCharacter->GetISkeletonPose()->SetDefaultPose();
			m_ModelViewPort->SetPlayerControl(PathFollowingStatus<<1);
		}
		else
		{
			//normal character viewer
			m_ModelViewPort->SetPlayerControl(0);
			pCharacter->GetISkeletonPose()->SetDefaultPose();
			Vec3 VPos=Vec3(0,3,2);
			Vec3 VTar=Vec3(0,0,1);
			Matrix33 orient; orient.SetRotationVDir( (VTar-VPos).GetNormalized() );
			m_ModelViewPort->m_Camera.SetMatrix( Matrix34(orient,VPos)  );
		}
	}
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
void CharPanel_Animation::OnButton_Idle2Move()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );

	m_ModelViewPort->SetPlayerPos(Vec3(ZERO));
	uint32 Idle2MoveStatus = m_Idle2Move.GetState() & BST_CHECKED;


	ICharacterInstance* pCharacter = m_ModelViewPort->GetCharacterBase();
	if (pCharacter)
	{
		if (Idle2MoveStatus)
		{
			//enable path-following
			pCharacter->GetISkeletonPose()->SetDefaultPose();
			m_ModelViewPort->SetPlayerControl(Idle2MoveStatus<<2);
		}
		else
		{
			//normal character viewer
			m_ModelViewPort->SetPlayerControl(0);
			pCharacter->GetISkeletonPose()->SetDefaultPose();
			Vec3 VPos=Vec3(0,3,2);
			Vec3 VTar=Vec3(0,0,1);
			Matrix33 orient; orient.SetRotationVDir( (VTar-VPos).GetNormalized() );
			m_ModelViewPort->m_Camera.SetMatrix( Matrix34(orient,VPos)  );
		}
	}
}


//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
void CharPanel_Animation::OnButton_IdleStep()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );

	m_ModelViewPort->SetPlayerPos(Vec3(ZERO));
	uint32 IdleStepStatus = m_IdleStep.GetState() & BST_CHECKED;

	ICharacterInstance* pCharacter = m_ModelViewPort->GetCharacterBase();
	if (pCharacter)
	{
		if (IdleStepStatus)
		{
			//enable path-following
			pCharacter->GetISkeletonPose()->SetDefaultPose();
			m_ModelViewPort->SetPlayerControl(IdleStepStatus<<3);
		}
		else
		{
			//normal character viewer
			m_ModelViewPort->SetPlayerControl(0);
			pCharacter->GetISkeletonPose()->SetDefaultPose();
			Vec3 VPos=Vec3(0,3,2);
			Vec3 VTar=Vec3(0,0,1);
			Matrix33 orient; orient.SetRotationVDir( (VTar-VPos).GetNormalized() );
			m_ModelViewPort->m_Camera.SetMatrix( Matrix34(orient,VPos)  );
		}
	}
}

void CharPanel_Animation::OnButton_LinearMorphSequence()
{
	AfxGetMainWnd()->SendMessage( WM_COMMAND,MAKEWPARAM(ID_ANIM_PLAY,0),(LPARAM)GetSafeHwnd() );
	uint32 status = m_LinearMorphSequence.GetState() & BST_CHECKED;
	m_LinearMorphSlider.EnableWindow( status );

}


////////////////////////////////////////////////////////////
void CharPanel_Animation::ClearAnims()
{
	if(m_ParentWnd)
		m_ParentWnd->GetAnimationPanel()->RemoveAll();	
}

void CharPanel_Animation::EnablePreset(bool bIsEnable)
{
	GetDlgItem(IDC_PRESET)->EnableWindow(bIsEnable);
	GetDlgItem(IDC_RECALCULATE)->EnableWindow(bIsEnable);
}


void CharPanel_Animation::SetPreset(CString name)
{
	if (name != "")
	{
		int ind = m_preset.FindStringExact(0, name);
		if(ind<0)
			ind = m_preset.AddString(name);
		m_preset.SetCurSel(ind);
	}
}


int CharPanel_Animation::GetSelectedAnimations() const
{
	std::vector<CString> names;
	if(m_ParentWnd)
		m_ParentWnd->GetAnimationPanel()->GetSelectedAnimations(names);

	return names.size();
}


//////////////////////////////////////////////////////////////////////////
void CharPanel_Animation::OnBnClickedRecalculate()
{
	//rc.exe pathtocbafile /file="outputfilename"
	//	m_animLoader->SaveDescription(PathUtil::GetGameFolder() + CBA_FILE);
	m_ModelViewPort->GetCharPanelPreset()->SaveCBAFile();

	CString strAnimName = GetCurrAnimName();
	if(!strlen(strAnimName))
		return;

	PROCESS_INFORMATION   pi;
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	char pRun[MAX_PATH];
	sprintf(pRun,"rc/rc.exe %s%s /file=\"%s\" /wait", PathUtil::GetGameFolder().c_str(), CBA_FILE, strAnimName);

	if(CreateProcess(0, pRun, NULL,NULL, FALSE, NORMAL_PRIORITY_CLASS,NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	// reload character
	//	m_ModelViewPort->GetCharacterBase()->GetISkeleton()->ReloadCAF(strAnimName);
	m_ModelViewPort->GetCharacterBase()->GetIAnimationSet()->ReloadCAF(strAnimName, false);
}

void CharPanel_Animation::OnPresetSelect()
{
	int ind = m_preset.GetCurSel();
	if(ind>=0)
	{
		CString str;
		m_preset.GetLBText(ind, str);
		if(m_ModelViewPort->GetCharPanelPreset())
			m_ModelViewPort->GetCharPanelPreset()->ChangeCurPreset(str);
	}
}


BOOL CharPanel_Animation::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	if(LOWORD(wParam)==RC_IDM_COPY_NAME)
	{
		CString strAnimName = GetCurrAnimName();
		if(strlen(strAnimName))
		{
			CClipboard clipboard;
			clipboard.PutString(strAnimName, strAnimName);
		}
	}
	return CWnd::OnCommand(wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////
void CharPanel_Animation::SelectAnimation( const CString &animation )
{

}

void CharPanel_Animation::OnNMCustomdrawInterpolationSlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}
