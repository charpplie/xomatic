/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
History:
- July 15th, 2009	-	Created by Michelle Martin

************************************************************************
************************************************************************/


#ifndef __ANIMATIONGRAPH2_DEFAULTS_DIALOG_H__
#define __ANIMATIONGRAPH2_DEFAULTS_DIALOG_H__

#pragma once

#include "Resource.h"
#include "Controls\TemplDef.h" // message map extensions for templates
#include "ToolbarDialog.h"
#include "Controls\PropertyCtrl.h"

#include "CharacterEditor\CharacterEditor.h"
#include "AnimationGraph_2.h"
#include "..\HyperGraphView.h"

// forward declare all the panels
class CAnimationNodeDefaultsPanel;
class CGeneralNodeDefaultsPanel;
class CMovementNodeDefaultsPanel;

// the actual dialog window
class CAnimationGraph2DefaultsDialog : public CDialog
{
public:
	CAnimationGraph2DefaultsDialog();
	virtual ~CAnimationGraph2DefaultsDialog() {};
	void	SetAnimationGraph(CAnimationGraph2Ptr pAnimGraph);
	
	void	DoDataExchange(CDataExchange* pDX);
	BOOL	OnInitDialog();
	void	OnOK();
	void	OnCancel();
	void	OnActiveTabChanged(NMHDR *pNMHDR, LRESULT *pResult);



protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSelectionChange();

private:

	CAnimationGraph2Ptr	m_pAnimGraph;

	CTabCtrl	m_dialogTabCtrl;
	int				m_currSelectedTab;
	CRect			m_ClientRect;

	CAnimationNodeDefaultsPanel*	m_animationNodeDefaults;
	CGeneralNodeDefaultsPanel*		m_graphNodeDefaults;
	CMovementNodeDefaultsPanel*		m_movementNodeDefaults;
};



// the classes for the different tab panels

class CAnimationNodeDefaultsPanel : public CDialog
{
public:
	CAnimationNodeDefaultsPanel();
	virtual ~CAnimationNodeDefaultsPanel() {};
	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();

	void ReadFromGraphDefaults(CAnimationGraph2Ptr pAnimGraph);
	void WriteToGraphDefaults(CAnimationGraph2Ptr pAnimGraph);
	void GrayOutAllElements(bool grayOut);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg	void OnLayerChanged();
	afx_msg void ResetToDefaultValues();

private:

	struct SDisplayDetails
	{
		void CopyFromStateDetails(const SAnimNodeDetails& pNodeDetails);
		void CopyToStateDetails(SAnimNodeDetails& pNodeDetails) const;

		CButton	m_ensureInStack;
		CButton	m_forceLeaveWhenFinished;
		CButton	m_stopCurrentAnimation;
		CButton	m_interruptCurrAnim;
		CButton	m_ManualUpdate;
		CButton	m_LoopAnimation;
		CButton	m_RepeatLastKey;
		CButton	m_AllowAnimRestart;
		CButton	m_VTimeWarping;
		CButton	m_Idle2Move;
		CButton	m_Move2Idle;
		CButton	m_PartialBodyUpdate;
		CButton	m_dontInterrupt;
		//CButton	m_WaitForAnimation;

		CEdit m_stickyOutTime;
		CEdit m_speedMultiplier;
		CEdit m_MPSpeedMultiplier;
		CEdit m_stayInStateUntil;
		//CEdit m_forceStayInStateUntil;
		CEdit m_transitionTime;
		CEdit m_keyTime;
	};

	CEdit	m_layer;
	int m_defaultLayer; // 1-based layer index (-1 means 'uninitialized')
	SDisplayDetails m_displayDetails;
	CAnimationGraph2Ptr	m_pAnimGraph;
};

//////////////////////////////////////////////////////////////////////////

class CGeneralNodeDefaultsPanel : public CDialog
{
public:
	CGeneralNodeDefaultsPanel();
	virtual ~CGeneralNodeDefaultsPanel() {};
	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();

	void ReadFromGraphDefaults(CAnimationGraph2Ptr pAnimGraph);
	void WriteToGraphDefaults(CAnimationGraph2Ptr pAnimGraph);
	void GrayOutAllElements(bool grayOut);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnCharacterBrowse();
	afx_msg void ResetToDefaultValues();

private:

	struct SGraphDisplayDetails
	{
		void CopyFromStateDetails(const SGraphNodeDetails& pNodeDetails);
		void CopyToStateDetails(SGraphNodeDetails& pNodeDetails) const;

		CButton	m_allowSelect;
		CButton	m_includeInGame;
		CButton	m_canMix;
		CButton	m_bHurryable;
		CButton	m_bSkipFP;
	};

	SGraphDisplayDetails	m_graphDisplayDetails;
	CEdit		m_characterFileName;
	CButton m_useExpensiveStateSelection_Ctrl;
	CAnimationGraph2Ptr	m_pAnimGraph;

};

//////////////////////////////////////////////////////////////////////////

class CMovementNodeDefaultsPanel : public CDialog
{
public:
	CMovementNodeDefaultsPanel();
	virtual ~CMovementNodeDefaultsPanel() {};
	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();

	void ReadFromGraphDefaults(CAnimationGraph2Ptr pAnimGraph);
	void WriteToGraphDefaults(CAnimationGraph2Ptr pAnimGraph);
	void GrayOutAllElements(bool grayOut);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void ResetToDefaultValues();

private:

	struct SMCMCollDetails
	{
		CComboBox	m_colliderMode;

		CComboBox	m_verticalMCM;
		CComboBox	m_horizontalMCM;

		CEdit	m_angle;
		CEdit	m_distance;
	};

	SMCMCollDetails	m_panelItems;
	CAnimationGraph2Ptr	m_pAnimGraph;

};


#endif 

