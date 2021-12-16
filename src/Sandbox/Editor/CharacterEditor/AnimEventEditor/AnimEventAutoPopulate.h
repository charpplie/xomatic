//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Karim Shakankiri
//---------------------------------------------------------------------------
#ifndef __ANIMEVENTAUTOPOPULATE_H__
#define __ANIMEVENTAUTOPOPULATE_H__

#include <CryExtension/CryCreateClassInstance.h>

class CAnimationControlDlg;

class CAnimEventAutoPopulate : public CDialog
{

public:
	CAnimEventAutoPopulate(CAnimationControlDlg* pParentWindow);

protected: 

	int GetSliderInFrames() const;
	void SetFoleyTimeTextFromSliderPos(int nPos);

	float GetFootHeight() const;
	void SetFootHeightText(float fHeight);

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnGenerateEventsBtn();


	IAnimationOperatorQueuePtr	m_pOperatorQueue; 

	CButton											m_generateFoleysCheck;

	CEdit												m_leftFootEventName;
	CEdit												m_rightFootEventName;
	CEdit												m_foleyEventName;

	CComboBox										m_leftFootBoneCombo;
	CComboBox										m_rightFootBoneCombo;
	CComboBox										m_foleyBoneCombo;

	CEdit												m_leftFootEventParam;
	CEdit												m_rightFootEventParam;
	CEdit												m_foleyEventParam;

	CSliderCtrl									m_foleyTimeDelaySlider;
	CEdit												m_foleyTimeDelayText;

	CSliderCtrl									m_footHeightSlider;
	CEdit												m_footHeightText;

	CButton											m_generateEventsBttn;


	float												m_fFootHeight;

	CAnimationControlDlg*				m_pAnimationControlDlg;
};


#endif