//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Karim Shakankiri
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include "AnimEventAutoPopulate.h"

#include "CharacterEditor/AnimEventEditor/AnimEventEditor.h"
#include "CharacterEditor/CharPanel_AnimationControl.h"
#include "CharacterEditor/ModelViewportCE.h"

// 
struct SEventEntry
{
	CString sAnimationPath;
	CString sEventName; 
	float fTime;
	CString sBoneName; 
	CString sParam;
};


//
CAnimEventAutoPopulate::CAnimEventAutoPopulate(CAnimationControlDlg* pParentWindow)
: CDialog(IDD_CHARACTER_EDITOR_AUTO_EVENTS, pParentWindow)
, m_pAnimationControlDlg(pParentWindow)
, m_fFootHeight(0.1f)
{
	::CryCreateClassInstance("AnimationPoseModifier_OperatorQueue", m_pOperatorQueue);
}

void CAnimEventAutoPopulate::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX,	IDC_FOLEYS_CHECK,					m_generateFoleysCheck);

	DDX_Control(pDX,	IDC_LEFT_FOOT_EVENT,			m_leftFootEventName);
	DDX_Control(pDX,	IDC_RIGHT_FOOT_EVENT,			m_rightFootEventName);
	DDX_Control(pDX,	IDC_FOLEY_EVENT,					m_foleyEventName);

	DDX_Control(pDX,	IDC_LEFT_BONE_COMBO,			m_leftFootBoneCombo);
	DDX_Control(pDX,	IDC_RIGHT_BONE_COMBO,			m_rightFootBoneCombo);
	DDX_Control(pDX,	IDC_FOLEY_BONE_COMBO,			m_foleyBoneCombo);

	DDX_Control(pDX,	IDC_LEFT_FOOT_PARAM,			m_leftFootEventParam);
	DDX_Control(pDX,	IDC_RIGHT_FOOT_PARAM,			m_rightFootEventParam);
	DDX_Control(pDX,	IDC_FOLEY_PARAM,					m_foleyEventParam);

	DDX_Control(pDX,	IDC_FOLEY_DELAY_TXT,			m_foleyTimeDelayText);
	DDX_Control(pDX,	IDC_FOLEY_DELAY_SLIDER,		m_foleyTimeDelaySlider);
	
	DDX_Control(pDX,	IDC_FOOT_HEIGHT_SLIDER,		m_footHeightSlider);
	DDX_Control(pDX,	IDC_FOOTSTEP_HEIGHT,			m_footHeightText);

	DDX_Control(pDX,	IDC_GENERATE_EVENTS,			m_generateEventsBttn);
}


BEGIN_MESSAGE_MAP(CAnimEventAutoPopulate, CDialog)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_GENERATE_EVENTS, OnGenerateEventsBtn)
END_MESSAGE_MAP()


BOOL CAnimEventAutoPopulate::OnInitDialog()
{
	// Call the base class implementation.
	CDialog::OnInitDialog();

	SetWindowPos(&this->wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);

	m_generateFoleysCheck.SetCheck(1);

	CRect sliderRect;
	m_foleyTimeDelaySlider.GetClientRect(&sliderRect);

	m_foleyTimeDelaySlider.SetParent(this);
	m_foleyTimeDelaySlider.SetOwner(this);

	m_foleyTimeDelaySlider.SetRange(0, 40, 1);
	m_foleyTimeDelaySlider.SetPos(13);

	m_foleyTimeDelayText.SetReadOnly();
	SetFoleyTimeTextFromSliderPos(GetSliderInFrames());

	m_footHeightSlider.SetParent(this);
	m_footHeightSlider.SetOwner(this);

	m_footHeightSlider.SetRange(0, 1000, 1);
	m_footHeightSlider.SetPos(110);

	m_footHeightText.SetReadOnly();
	SetFootHeightText(GetFootHeight());

	m_leftFootEventName.SetWindowText("footstep");
	m_rightFootEventName.SetWindowText("footstep");
	m_foleyEventName.SetWindowText("foley");

	CRY_ASSERT(NULL != m_pAnimationControlDlg && NULL != m_pAnimationControlDlg->m_pModelViewportCE);

	std::vector<string> bones;
	m_pAnimationControlDlg->m_pModelViewportCE->GetJointNameList(bones);

	m_leftFootBoneCombo.AddString("<none>");
	m_rightFootBoneCombo.AddString("<none>");
	m_foleyBoneCombo.AddString("<none>");

	CDC* dc = m_leftFootBoneCombo.GetDC();
	UINT textExtent = 0;
	for(std::vector<string>::const_iterator itS = bones.begin(); itS != bones.end(); ++itS)
	{
		if( dc )
		{
			CString itemString = *itS;
			UINT nWidth = dc->GetTextExtent(itemString).cx;
			textExtent = max(nWidth, textExtent);
		}
		m_leftFootBoneCombo.AddString(itS->c_str());
		m_rightFootBoneCombo.AddString(itS->c_str());
		m_foleyBoneCombo.AddString(itS->c_str());
	}

	m_leftFootBoneCombo.SetCurSel(m_leftFootBoneCombo.FindString(-1, "L_Foot"));
	m_rightFootBoneCombo.SetCurSel(m_rightFootBoneCombo.FindString(-1, "R_Foot"));
	m_foleyBoneCombo.SetCurSel(m_foleyBoneCombo.FindString(-1, "Spine02"));

	// Set drop-down list width to be wider than the bone names.
	textExtent += ::GetSystemMetrics(SM_CXVSCROLL) + 2*::GetSystemMetrics(SM_CXEDGE);
	UINT newDroppedWidth = max(textExtent, m_leftFootBoneCombo.GetHorizontalExtent());
	m_leftFootBoneCombo.SetDroppedWidth(newDroppedWidth);
	m_rightFootBoneCombo.SetDroppedWidth(newDroppedWidth);
	m_foleyBoneCombo.SetDroppedWidth(newDroppedWidth);

	m_leftFootEventParam.SetWindowText("");
	m_rightFootEventParam.SetWindowText("");
	m_foleyEventParam.SetWindowText("");

	return TRUE;
}


//////////////////////////////////////////////////////////////////////////
int CAnimEventAutoPopulate::GetSliderInFrames() const
{
	return m_foleyTimeDelaySlider.GetPos() - 10;
}

//////////////////////////////////////////////////////////////////////////
void CAnimEventAutoPopulate::SetFoleyTimeTextFromSliderPos(int nPos)
{
	char szTempStr[32];
	sprintf(szTempStr, "%d frames (%.2f s)\0", nPos, float(nPos) / 30.0f);
	m_foleyTimeDelayText.SetWindowText(szTempStr);
}

//////////////////////////////////////////////////////////////////////////
float CAnimEventAutoPopulate::GetFootHeight() const
{
	return float(m_footHeightSlider.GetPos()) * 0.001f;
}

//////////////////////////////////////////////////////////////////////////
void CAnimEventAutoPopulate::SetFootHeightText(float fHeight)
{
	char szTempStr[32];
	sprintf(szTempStr, "%.3f\0", fHeight);
	m_footHeightText.SetWindowText(szTempStr);
}


//////////////////////////////////////////////////////////////////////////
void CAnimEventAutoPopulate::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if ((CSliderCtrl*) (pScrollBar) == &m_foleyTimeDelaySlider)
	{
		int nPos = GetSliderInFrames();
		SetFoleyTimeTextFromSliderPos(nPos);
	}
	if ((CSliderCtrl*) (pScrollBar) == &m_footHeightSlider)
	{
		float fHeight = GetFootHeight();
		SetFootHeightText(fHeight);
	}
}


//////////////////////////////////////////////////////////////////////////
void CAnimEventAutoPopulate::OnGenerateEventsBtn()
{
	CModelViewportCE* pModelViewportCE = m_pAnimationControlDlg->m_pModelViewportCE;
	CRY_ASSERT(NULL != m_pAnimationControlDlg && NULL != m_pAnimationControlDlg->m_pAnimEventEditor && NULL != pModelViewportCE);
	if (NULL == pModelViewportCE)
	{
		return;
	}

	ICharacterInstance* pCharInst = pModelViewportCE->GetCharacterBase();
	if (NULL == pCharInst)
	{
		return;
	}

	// Layer
	const int currentActiveAnimationLayer = pModelViewportCE->GetCurrentActiveAnimationLayer();

	// Character path (to create intermediate char instances)
	const char* szFilePath = pCharInst->GetFilePath();

	// Get bone IDs from the combo boxes
	CString sLeftBoneName;
	m_leftFootBoneCombo.GetLBText(m_leftFootBoneCombo.GetCurSel(), sLeftBoneName);
	const uint32 nLeftFootIdx = pCharInst->GetIDefaultSkeleton().GetJointIDByName(sLeftBoneName.GetString());

	CString sRightBoneName;
	m_rightFootBoneCombo.GetLBText(m_rightFootBoneCombo.GetCurSel(), sRightBoneName);
	const uint32 nRightFootIdx = pCharInst->GetIDefaultSkeleton().GetJointIDByName(sRightBoneName.GetString());

	CString sFoleyBoneName;
	m_foleyBoneCombo.GetLBText(m_foleyBoneCombo.GetCurSel(), sFoleyBoneName);

	// Get animation name / path and length
	const string sAnimName = pModelViewportCE->GetAnimationNameByLayer(currentActiveAnimationLayer);
	const string sAnimationPath = pModelViewportCE->GetAnimationPathByLayer(currentActiveAnimationLayer);

	// Create a character instance for the frame
	pCharInst = gEnv->pCharacterManager->CreateInstance(szFilePath);
	CRY_ASSERT(pCharInst);
	if (NULL == pCharInst)
	{
		return;	
	}
	pCharInst->AddRef();

	// 
	ISkeletonAnim* pISkeletonAnim = pCharInst->GetISkeletonAnim();
	ISkeletonPose* pISkeletonPose = pCharInst->GetISkeletonPose();
	IDefaultSkeleton& rIDefaultSkeleton = pCharInst->GetIDefaultSkeleton();
	IAnimationSet* pAnimSet = pCharInst->GetIAnimationSet();
	if (NULL == pISkeletonAnim || NULL == pISkeletonPose || NULL == pAnimSet)
	{
		return;
	}


	const int nAnimID = pAnimSet->GetAnimIDByName(sAnimName.c_str());
	if (sAnimationPath.empty() || sAnimName.empty() || nAnimID < 0)
	{
		return;
	}

	// Play the animation
	CryCharAnimationParams AParams(currentActiveAnimationLayer);
	AParams.m_nFlags = CA_REPEAT_LAST_KEY|CA_MANUAL_UPDATE|CA_ALLOW_ANIM_RESTART;
	AParams.m_fTransTime = 0.0f;
	AParams.m_fKeyTime = 0.0f;
	pISkeletonAnim->StartAnimationById(nAnimID, AParams);
		
	const float fLen = pISkeletonAnim->GetAnimFromFIFO(0, 0).GetExpectedTotalDurationSeconds();
	
	// Number of samples we'll use (assume 30fps). Round up
	const uint32 nbFrames = (fLen * 30.0f) + 0.5f;
	std::vector<QuatT> nLeftSamples(nbFrames);
	std::vector<QuatT> nRightSamples(nbFrames);

	// For each frame, compute pose. 
	const float fInvNbFrames = 1.0f / float(nbFrames);
	for (uint32 nFrame = 0; nFrame < nbFrames; ++nFrame)
	{
		const float fNormalizedTime = float(nFrame) * fInvNbFrames;

		pCharInst->SetCharEditMode(1);
		pISkeletonPose->SetForceSkeletonUpdate(1);

		pISkeletonAnim->SetLayerNormalizedTime(0, fNormalizedTime);
		
		// Play animation
		SAnimationProcessParams params;
		params.bOnRender = false;
		params.locationAnimation = QuatTS(IDENTITY);
		params.zoomAdjustedDistanceFromCamera = 1.0f;
		pCharInst->StartAnimationProcessing(params);

		// Directly compute and get the result on this thread
		pCharInst->FinishAnimationComputations();

		nLeftSamples[nFrame] = pISkeletonPose->GetAbsJointByID(nLeftFootIdx);
		nRightSamples[nFrame] = pISkeletonPose->GetAbsJointByID(nRightFootIdx);
	}

	// Analysis. Compute left and right foot step events
	const float fFootDownHeight = GetFootHeight();

	std::set<float> vLeftFootstepTimes;
	std::set<float> vRightFootstepTimes;

	bool fLeftFootDown = nLeftSamples.front().t.z < fFootDownHeight;
	bool fRightFootDown = nRightSamples.front().t.z < fFootDownHeight;
	for (uint32 nFrame = 1; nFrame < nbFrames; ++nFrame)
	{

		const bool bCurrLeftDown = nLeftSamples[nFrame].t.z < fFootDownHeight;
		const bool bCurrRightDown = nRightSamples[nFrame].t.z < fFootDownHeight;

		if (!fLeftFootDown && bCurrLeftDown)
		{
			vLeftFootstepTimes.insert(float(nFrame) * fInvNbFrames);
		}

		if (!fRightFootDown && bCurrRightDown)
		{
			vRightFootstepTimes.insert(float(nFrame) * fInvNbFrames);
		}

		fLeftFootDown = bCurrLeftDown;
		fRightFootDown = bCurrRightDown;
	}

	const bool bGenerateFoleys = m_generateFoleysCheck.GetCheck();
	const float fFoleyTimeDelay = GetSliderInFrames() / ( 30.0f * fLen);

	for (std::set<float>::const_iterator itSLeft = vLeftFootstepTimes.begin(); itSLeft != vLeftFootstepTimes.end(); ++itSLeft)
	{
		CString sEventName;
		m_leftFootEventName.GetWindowText(sEventName);

		CString sParam;
		m_leftFootEventParam.GetWindowText(sParam);
		m_pAnimationControlDlg->m_pAnimEventEditor->AddEvent(sAnimationPath.c_str(), sEventName.GetString(), *itSLeft, sLeftBoneName.GetString(), sParam.GetString());

		if (bGenerateFoleys)
		{
			CString sFoleyEventName;
			m_foleyEventName.GetWindowText(sFoleyEventName);

			CString sFoleyParam;
			m_foleyEventParam.GetWindowText(sFoleyParam);
			const float fTimeLeft = clamp_tpl(*itSLeft + fFoleyTimeDelay, 0.0f, fLen);
			m_pAnimationControlDlg->m_pAnimEventEditor->AddEvent(sAnimationPath.c_str(), sFoleyEventName.GetString(), fTimeLeft, sFoleyBoneName.GetString(), sFoleyParam.GetString());
		}
	}
	 
	for (std::set<float>::const_iterator itSRight = vRightFootstepTimes.begin(); itSRight != vRightFootstepTimes.end(); ++itSRight)
	{
		CString sEventName;
		m_rightFootEventName.GetWindowText(sEventName);

		CString sParam;
		m_rightFootEventParam.GetWindowText(sParam);
		m_pAnimationControlDlg->m_pAnimEventEditor->AddEvent(sAnimationPath.c_str(), sEventName.GetString(), *itSRight, sRightBoneName.GetString(), sParam.GetString());

		if (bGenerateFoleys)
		{
			CString sFoleyEventName;
			m_foleyEventName.GetWindowText(sFoleyEventName);

			CString sFoleyParam;
			m_foleyEventParam.GetWindowText(sFoleyParam);
			const float fTimeRight = clamp_tpl((*itSRight + fFoleyTimeDelay), 0.0f, fLen);
			m_pAnimationControlDlg->m_pAnimEventEditor->AddEvent(sAnimationPath.c_str(), sFoleyEventName.GetString(), fTimeRight, sFoleyBoneName.GetString(), sFoleyParam.GetString());
		}
	}

	pCharInst->Release();
}