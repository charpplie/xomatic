#if !defined(AFX_MODELVIEWPANEL2_H__3C441EB6_B02A_4D2F_A5E9_587E334E9F48__INCLUDED_)
#define AFX_MODELVIEWPANEL2_H__3C441EB6_B02A_4D2F_A5E9_587E334E9F48__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Controls\AnimSequences.h"
#include "Controls\FillSliderCtrl.h"

class CCharacterEditor;

class CharPanel_Animation;

/////////////////////////////////////////////////////////////////////////////
// CharPanel_Animation dialog
/////////////////////////////////////////////////////////////////////////////

class ICharacterChangeListener;
class SANDBOX_API CharPanel_Animation : public CDialog
{
#define MAX_PMG_PARAM_DIM (5)

public:
	CharPanel_Animation( class CModelViewport *vp,CWnd* pParent = NULL);   // standard constructor
	virtual ~CharPanel_Animation();

	enum { IDD = IDD_CHARPANEL_ANIMATION };

	void ResetPMGControls();
	void SetBaseCharacter();
	void SetFileName( const CString &filename );

	void DisableFileBroser() { m_browseFileBtn.EnableWindow(FALSE); }
	void ClearAnims();

	CString GetCurrFileName();
	CString GetCurrAnimName(int nNum = 0);
	bool GetAnimLightState();
	void ClearBones();
	void AddBone( const CString &bone );
	void SelectBone( const CString &bone );

	f32 GetBlendSpaceFlexX(f32 sec, f32 aft); 
	f32 GetBlendSpaceFlexY(f32 sec, f32 aft); 
	f32 GetBlendSpaceFlexZ(f32 sec, f32 aft); 


	//------------------------------------------------------------------------------
	// Current character path
	const CString GetCurrentCharacterPath(){CString str; m_fileEdit.GetWindowText(str); return str;}
	void SetCurrentCharacterPath(const CString strPath){m_fileEdit.SetWindowText(strPath);}

	//------------------------------------------------------------------------------
	// Layer
	int GetLayer() const { return m_layer.GetCurSel(); }
	void SetLayer(const int32 layInd) {m_layer.SetCurSel(layInd);}

	//------------------------------------------------------------------------------
	// Speed turn lock
	bool GetDesiredLocomotionSpeed() { return m_DesiredLocomotionSpeed.GetState() & BST_CHECKED;	}
	void SetDesiredLocomotionSpeed(const BOOL bSet) {m_DesiredLocomotionSpeed.SetCheck(bSet);}
	bool GetDesiredTurnSpeed() { return m_DesiredTurnSpeed.GetState() & BST_CHECKED;	}
	void SetDesiredTurnSpeed(const BOOL bSet) {m_DesiredTurnSpeed.SetCheck(bSet);};
	bool GetLockMoveBody() { return m_LockMoveBody.GetState() & BST_CHECKED;	}
	void SetLockMoveBody(const BOOL bSet) {m_LockMoveBody.SetCheck(bSet);}

	//------------------------------------------------------------------------------
	// Blend-space Control
	f32 GetBlendSpaceSliderX() { return m_BlendSpaceSliderX.GetValue(); }
	f32 GetBlendSpaceSliderY() { return m_BlendSpaceSliderY.GetValue(); }
	f32 GetBlendSpaceSliderZ() { return m_BlendSpaceSliderZ.GetValue(); }

	void SetBlendSpaceSliderX(f32 val) { 
		m_BlendSpaceSliderX.SetValue(val); 
		m_BlendSpaceNumberX.SetValue(val);
	}
	void SetBlendSpaceSliderY(f32 val) { 
		m_BlendSpaceSliderY.SetValue(val);  
		m_BlendSpaceNumberY.SetValue(val);
	}
	void SetBlendSpaceSliderZ(f32 val) { 
		m_BlendSpaceSliderZ.SetValue(val);  
		m_BlendSpaceNumberZ.SetValue(val);
	}

	void EnableWindow_BlendSpaceSliderX(uint32 i)	{
		m_BlendSpaceSliderX.EnableWindow(i);
		m_BlendSpaceNumberX.EnableWindow(i);
	}
	void EnableWindow_BlendSpaceSliderY(uint32 i)	{
		m_BlendSpaceSliderY.EnableWindow(i);
		m_BlendSpaceNumberY.EnableWindow(i);
	}
	void EnableWindow_BlendSpaceSliderZ(uint32 i)	{
		m_BlendSpaceSliderZ.EnableWindow(i);
		m_BlendSpaceNumberZ.EnableWindow(i);
	}

	//------------------------------------------------------------------------------
	// Animation driven and allow vertical movement
	bool GetAnimationDrivenMotion() { 	return m_AnimationDrivenMotion.GetState() & BST_CHECKED;	}
	void SetAnimationDrivenMotion(const BOOL bSet) {m_AnimationDrivenMotion.SetCheck(bSet);}
	bool GetVerticalMovement() { 	return m_VerticalMovement.GetState() & BST_CHECKED;	}
	void SetVerticalMovement(const BOOL bSet) {m_VerticalMovement.SetCheck(bSet);}

	//------------------------------------------------------------------------------
	// Animation flags
	bool GetMirrorAnimation() { return m_MirrorAnimation.GetState() & BST_CHECKED;	}
	void SetMirrorAnimation(const BOOL bSet){m_MirrorAnimation.SetCheck(bSet);}
	bool GetLoopAnimation() { return m_LoopAnimation.GetState() & BST_CHECKED;	}
	void SetLoopAnimation(const BOOL bSet) {m_LoopAnimation.SetCheck(bSet);};
	bool GetRepeatLastKey() { return m_RepeatLastKey.GetState() & BST_CHECKED;	}
	void SetRepeatLastKey(const BOOL bSet) {m_RepeatLastKey.SetCheck(bSet);}
	bool GetVTimeWarping(){ return m_VTimeWarping.GetState() & BST_CHECKED;	}
	void SetVTimeWarping(const BOOL bSet) {m_VTimeWarping.SetCheck(bSet);}
	bool GetTransitionTimeCheck() {return m_useTransitionTime.GetState() & BST_CHECKED;}
	void SetTransitionTimeCheck(const BOOL bSet) {m_useTransitionTime.SetCheck(bSet);}
	f32 GetTransitionTimeValue() { return (m_useTransitionTime.GetState()&BST_CHECKED) ? m_TransitionTimeControl.GetValue() : -1.0f; 	}
	void SetTransitionTimeValue(const f32 mValue) {m_TransitionTimeControl.SetValue((double)mValue);}
	bool GetDisableMultilayer() { return m_DisableMultilayerAnimation.GetState() & BST_CHECKED;	}
	void SetDisableMultilayer(const BOOL bSet) {m_DisableMultilayerAnimation.SetCheck(bSet);}
	bool GetAllowAnimationRestart() { return m_AllowAnimationRestart.GetState() & BST_CHECKED;	}
	void SetAllowAnimationRestart(const BOOL bSet){m_AllowAnimationRestart.SetCheck(bSet);}
	uint32 GetAimIKLayerValue() { return m_AimIKLayer.GetValue();	}
	void SetAimIKLayerValue(const uint32 layerValue){m_AimIKLayer.SetValue(layerValue);}
	f32 GetAdditiveWeight(uint32 nLayer) { return m_arrAdditiveWeightsSmooth[nLayer];	}
	void SetAdditiveWeight(const f32 weight) {m_AdditiveWeight.SetValue(weight);}

	//------------------------------------------------------------------------------
	// Post processing
	bool GetLookIK() { return m_LookIK.GetState() & BST_CHECKED; }
	void SetLookIK(const BOOL bSet) {m_LookIK.SetCheck(bSet);}
	bool GetAimIK() { return m_AimIK.GetState() & BST_CHECKED; }
	void SetAimIK(const BOOL bSet) {m_AimIK.SetCheck(bSet);}
	bool GetLFootIK() { return m_LFootIK.GetState() & BST_CHECKED; }
	void SetLFootIK(const BOOL bSet) {m_LFootIK.SetCheck(bSet);}
	bool GetRFootIK() { return m_RFootIK.GetState() & BST_CHECKED; }
	void SetRFootIK(const BOOL bSet) {m_RFootIK.SetCheck(bSet);}
	bool GetLArmIK() { return m_LArmIK.GetState() & BST_CHECKED; }
	void SetLArmIK(const BOOL bSet){m_LArmIK.SetCheck(bSet);}
	bool GetRArmIK() { return m_RArmIK.GetState() & BST_CHECKED; }
	void SetRArmIK(const BOOL bSet){m_RArmIK.SetCheck(bSet);}
	bool GetFootAnchoring(){	return m_FootAnchoring.GetState() & BST_CHECKED;	}
	void SetFootAnchoring(const BOOL bSet) {m_FootAnchoring.SetCheck(bSet);}
	bool GetGroundAlign() { return m_GroundAlign.GetState() & BST_CHECKED; }
	void SetGroundAlign(const BOOL bSet){m_GroundAlign.SetCheck(bSet);}

	//------------------------------------------------------------------------------
	// Animation speed control
	f32 GetAnimationProgress() {return m_AnimationSpeedSlider.GetValue();}
	void SetAnimationProgress(const f32 val) {m_AnimationSpeedSlider.SetValue(val); m_AnimationSpeedNumber.SetValue(val);}

	//------------------------------------------------------------------------------

	void EnableAdditiveWeightWindow(uint32 i) 
	{ 
		m_AdditiveWeight.EnableWindow(i);	
		m_AdditiveWeightText.EnableWindow(i);	
	}

	void SmoothAdditiveWeights(f32 fFrameTime) 
	{
		for (uint32 i=0; i<16; i++)
			SmoothCD(m_arrAdditiveWeightsSmooth[i], m_arrAdditiveWeightsSmoothRate[i], fFrameTime, m_arrAdditiveWeights[i], 0.50f);
	}

	//------------------------------------------------------------------------------
	// Unit Test for Humans
	bool GetPlayerControl() {return m_PlayControl.GetState() & BST_CHECKED;}
	void SetPlayerControl(const BOOL bSet) {m_PlayControl.SetCheck(bSet);}
	bool GetFixedCamera(){	return m_FixedCamera.GetState() & BST_CHECKED;	}
	void SetFixedCamera(const BOOL bSet) { m_FixedCamera.SetCheck(bSet);}
	bool GetPathFollowing() {return m_PathFollowing.GetState() & BST_CHECKED;}
	void SetPathFollowing(const BOOL bSet) {m_PathFollowing.SetCheck(bSet);}	
	bool GetAttachedCamera(){	return m_AttachedCamera.GetState() & BST_CHECKED;	}
	void SetAttachedCamera(const BOOL bSet) {m_AttachedCamera.SetCheck(bSet);}
	bool GetIdle2Move() {return m_Idle2Move.GetState() & BST_CHECKED;}
	void SetIdle2Move(const BOOL bSet) {m_Idle2Move.SetCheck(bSet);}
	bool GetIdleStep() {return m_IdleStep.GetState() & BST_CHECKED;}
	void SetIdleStep(const BOOL bSet) {m_IdleStep.SetCheck(bSet);}

	//------------------------------------------------------------------------------
	// Gamepad and morph
	bool GetGamepad() { 	return m_Gamepad.GetState() & BST_CHECKED;	}
	void SetGamepad(const BOOL bSet) {m_Gamepad.SetCheck(bSet);}
	bool GetLinearMorphSequence() { 	return m_LinearMorphSequence.GetState() & BST_CHECKED;	}
	void SetLinearMorphSequence(const BOOL bSet) {m_LinearMorphSequence.SetCheck(bSet);}
	bool GetUseMorphTargets() { 	return m_UseMorphTargets.GetState() & BST_CHECKED;	}
	void SetUseMorphTargets(const BOOL bSet) {m_UseMorphTargets.SetCheck(bSet);}

	//------------------------------------------------------------------------------
	// Linear morph slider
	f32 GetLinearMorphSlider() {	return m_LinearMorphSlider.GetValue();	}
	void SetLinearMorphSlider(const f32 val) {m_LinearMorphSlider.SetValue(val);}

	f32 GetLinearMorphSliderFlex(f32 sec, f32 aft); 

	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

	void EnablePreset(bool bIsEnable);
	void SetPreset(CString name);
	int GetSelectedAnimations() const;
	void SelectAnimation( const CString &animation );

	std::map<string, f32>& GetPrePmgParamMap() {return m_prePMGParams;}
	CFillSliderCtrl m_ControlParam[MAX_PMG_PARAM_DIM];
	CNumberCtrl m_ControlParamEdit[MAX_PMG_PARAM_DIM];
	CStatic m_ControlParamName[MAX_PMG_PARAM_DIM];
protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK() {};
	virtual void OnCancel() {};
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBrowseFile();
	afx_msg void OnBnClickedReloadFile();
	afx_msg void OnBnClickedMaterial();
	afx_msg void OnAnimateLights();

	afx_msg void OnButton_ChooseBaseCharacter();
	afx_msg void OnButton_StartSelectedAnimations();
	afx_msg void OnButton_SetAimPoses();

	afx_msg void OnButton_StopAnimationInLayer();
	afx_msg void OnButton_StopAnimationsAll();
	afx_msg void OnButton_ResetModel();

	afx_msg void OnButton_GetTransitionTime()	
	{	
		m_TransitionTimeControl.EnableWindow( (m_useTransitionTime.GetState()&BST_CHECKED) );	
	}
	afx_msg void OnNumber_GetAdditiveWeightValue()	
	{	
		uint32 layer = m_layer.GetCurSel(); 
		m_arrAdditiveWeights[layer]=m_AdditiveWeight.GetValue();
	}
	afx_msg void OnChange_GetLayer()	
	{	
		uint32 layer = m_layer.GetCurSel(); 
		f32 AddWeight = m_arrAdditiveWeights[layer];
		m_AdditiveWeight.SetValue(AddWeight);
	}

	afx_msg void OnButton_TransRot2000();
	afx_msg void OnButton_VerticalMovement();
	afx_msg void OnButton_VTimeWarping();
	afx_msg void OnButton_FootAnchoring();

	afx_msg void OnSlider_BlendSpaceX();
	afx_msg void OnNumber_BlendSpaceX();
	afx_msg void OnSlider_BlendSpaceY();
	afx_msg void OnNumber_BlendSpaceY();
	afx_msg void OnSlider_BlendSpaceZ();
	afx_msg void OnNumber_BlendSpaceZ();

	afx_msg void OnSlider_ControlParam1();
	afx_msg void OnNumber_ControlParam1();
	afx_msg void OnSlider_ControlParam2();
	afx_msg void OnNumber_ControlParam2();
	afx_msg void OnSlider_ControlParam3();
	afx_msg void OnNumber_ControlParam3();
	afx_msg void OnSlider_ControlParam4();
	afx_msg void OnNumber_ControlParam4();
	afx_msg void OnSlider_ControlParam5();
	afx_msg void OnNumber_ControlParam5();


	afx_msg void OnSlider_PolarX();
	afx_msg void OnNumber_PolarX();
	afx_msg void OnSlider_PolarY();
	afx_msg void OnNumber_PolarY();


	afx_msg void OnSlider_MotionInterpolation();
	afx_msg void OnSlider_ManualUpdate();


	afx_msg void OnSlider_AnimationSpeed();
	afx_msg void OnNumber_AnimationSpeed();


	afx_msg void OnLoop();
	afx_msg void OnButton_PlayerControl();
	afx_msg void OnButton_PathFollowing();
	afx_msg void OnButton_Idle2Move();
	afx_msg void OnButton_IdleStep();


	afx_msg void OnButton_LFootIK();
	afx_msg void OnButton_RFootIK();
	afx_msg void OnButton_LArmIK();
	afx_msg void OnButton_RArmIK();
	afx_msg void OnButton_GroundAlign();


	afx_msg void OnButton_LinearMorphSequence();
	afx_msg void OnSlider_LinearMorph();

	afx_msg void OnBnClickedRecalculate();
	afx_msg void OnPresetSelect();

	DECLARE_MESSAGE_MAP()

	CEdit m_fileEdit;
	CModelViewport *m_ModelViewPort;

	//CCustomButton m_materialBtn;
	CCustomButton m_reloadFileBtn;
	CCustomButton m_browseFileBtn;
	CCustomButton m_materialBtn;

	CCustomButton m_ChooseBaseCharacter;


	uint32 m_SelectedAnimations;
	CCustomButton m_StartSelectedAnimations;
	CCustomButton m_SetAimPoses;
	CCustomButton m_StopAnimationLayer;


	CFillSliderCtrl m_BlendSpaceSliderX;
	CNumberCtrl m_BlendSpaceNumberX;
	CFillSliderCtrl m_BlendSpaceSliderY;
	CNumberCtrl m_BlendSpaceNumberY;
	CFillSliderCtrl m_BlendSpaceSliderZ;
	CNumberCtrl m_BlendSpaceNumberZ;

	CFillSliderCtrl m_PolarX_Slider;
	CNumberCtrl m_PolarX_Number;
	CFillSliderCtrl m_PolarY_Slider;
	CNumberCtrl m_PolarY_Number;

	CButton m_DesiredLocomotionSpeed;
	CButton m_DesiredTurnSpeed;
	CButton m_LockMoveBody;


	// flags per instance
	CButton m_AnimationDrivenMotion;
	CButton m_VerticalMovement;

	// per animation flags
	CButton	m_MirrorAnimation;


	CButton	m_LoopAnimation;
	CButton	m_RepeatLastKey;
	CButton m_VTimeWarping;

	CNumberCtrl m_AdditiveWeight;
	CStatic m_AdditiveWeightText;
	f32 m_arrAdditiveWeights[16];
	f32 m_arrAdditiveWeightsSmooth[16];
	f32 m_arrAdditiveWeightsSmoothRate[16];

	CNumberCtrl m_AimIKLayer;

	CButton m_DisableMultilayerAnimation;
	CButton m_useTransitionTime; CNumberCtrl m_TransitionTimeControl;
	CButton	m_AllowAnimationRestart;


	// flags for post-processing
	CButton	m_LookIK; 				CButton	m_AimIK;
	CButton	m_LArmIK;					CButton	m_RArmIK;
	CButton	m_LFootIK;				CButton	m_RFootIK;
	CButton m_FootAnchoring;	CButton	m_GroundAlign;


	CFillSliderCtrl m_AnimationSpeedSlider;
	CNumberCtrl m_AnimationSpeedNumber;

	CButton m_Gamepad;
	CButton m_LinearMorphSequence;
	CButton m_UseMorphTargets;

	std::vector<f32> m_arrLinearMorphFade;
	CFillSliderCtrl m_LinearMorphSlider;
	CNumberCtrl m_LayerMotionInterpolationNumber;

	CComboBox	m_preset;

	std::map<string, f32> m_prePMGParams;

public:
	CComboBox	m_layer;
	CColoredListBox	m_animations;

	CButton m_PlayControl;
	CButton m_FixedCamera;

	CButton m_PathFollowing;
	CButton m_AttachedCamera;

	CButton m_Idle2Move;
	CButton m_IdleStep;

	CCharacterEditor* m_ParentWnd;

	afx_msg void OnNMCustomdrawInterpolationSlider(NMHDR *pNMHDR, LRESULT *pResult);
};


#endif // !defined(AFX_MODELVIEWPANEL_H__3C441EB6_B02A_4D2F_A5E9_587E334E9F48__INCLUDED_)
