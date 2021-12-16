
/************************************************************************/
/*     Represents the panel with the details for an animation node      */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_ANIMATION_NODE_DETAILS_H__
#define __ANIMATIONGRAPH2_ANIMATION_NODE_DETAILS_H__

#pragma once

#include "Resource.h"
#include "..\..\Controls\TemplDef.h" // message map extensions for templates
#include "..\ToolbarDialog.h"
#include "..\Controls\PropertyCtrl.h"

#include "..\CharacterEditor\CharacterEditor.h"
#include "AnimationGraph_2.h"
#include "..\HyperGraphView.h"


class CAnimationGraphAnimationNodeDetails_General;
class CAnimationGraphAnimationNodeDetails_MCMColl;

class CAnimationGraphAnimationNodeDetails : public CDialog
{
	DECLARE_DYNCREATE( CAnimationGraphAnimationNodeDetails );

public:
	CAnimationGraphAnimationNodeDetails();
	virtual ~CAnimationGraphAnimationNodeDetails() {};

	enum { IDD = IDD_AG2_ANIM_NODE_DETAILS };

	void	DoDataExchange(CDataExchange* pDX);
	void	Init();
	void	OnActiveTabChanged(NMHDR *pNMHDR, LRESULT *pResult);

	void	SetAnimationGraph(CAnimationGraph2Ptr pAnimGraph);
	void	SetParamsDeclaration( CParamsDeclaration2* pParamsDcl );
	void	SetActiveAnimNode( CAGState2Ptr animNode );

	//! Updates the details panel with the information from the node
	void UpdateInformationDisplay();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSelectionChange();

private:

	void DisableAllTabs();

	CAnimationGraph2Ptr	m_pAnimGraph;

	// pointer to the animation node that holds the actual information
	CAGState2Ptr m_pAnimNode;
	CParamsDeclaration2* m_pParamsDeclaration;

	CTabCtrl	m_dialogTabCtrl;
	int				m_currSelectedTab;
	CRect			m_ClientRect;

	CAnimationGraphAnimationNodeDetails_General*	m_animationTab;
	CAnimationGraphAnimationNodeDetails_MCMColl*	m_movementTab;
};


//////////////////////////////////////////////////////////////////////////
//           Tab Classes
//////////////////////////////////////////////////////////////////////////

class CAnimationGraphAnimationNodeDetails_General : public CDialog/*, public ISliderListener*/
{
	DECLARE_DYNCREATE( CAnimationGraphAnimationNodeDetails_General );

public:

	CAnimationGraphAnimationNodeDetails_General();
	
	~CAnimationGraphAnimationNodeDetails_General() {}

	enum { IDD = IDD_AG2_ANIM_NODE_DETAILS_GENERAL };

	void Init();
	void SetParamsDeclaration( CParamsDeclaration2* pParamsDcl );

	void SetActiveAnimNode( CAGState2Ptr animNode );

	//! Updates the details panel with the information from the node
	void UpdateInformationDisplay();

protected:
	DECLARE_MESSAGE_MAP()

	//! This sets up the correspondences between local variables 
	//! and the interface items in the panel, so that the data
	//! that is written in the variables will be displayed.	
	virtual void DoDataExchange( CDataExchange* pDX );
	void OnDetailsChanged( bool switchToCustomPreset = true );

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnUserChangedValues();
	afx_msg void OnNodeParamsChanged();
	afx_msg void OnAnimationNameChanged();
	afx_msg void OnStateNameChanged();
	afx_msg void OnLayerChanged();
	afx_msg void OnStayUntilSliderChanged();
	afx_msg void OnPresetSelected();
	afx_msg void OnUnHideParameters();


protected:

	// standard functions from CDialog

	virtual void OnOK() {}
	virtual void OnCancel() {}

private:

	void GrayOutNonParameterizedData(bool grayOut);

	//! Some parameters exclude others (for example StopCurrentAnim invalidates TransitionTime)
	//! This function will go through the current list and gray out the ones that are
	//! invalid. If you add parameters that exclude each other, add your code here.
	void UpdateParameterExclusions();

	void HideParameters(bool hide);

	// old stuff
	int	m_MiniPanelWidth;
	int	m_StateParamsDlgHeight;
	bool m_bIgnoreVScroll;
	CParamsDeclaration2* m_pParamsDeclaration;

	// pointer to the animation node that holds the actual information
	CAGState2Ptr m_pAnimNode;
	TPerParameterizationData m_displayParamedNodeDetails;


	// items on the panel
	//======================

	struct SDisplayDetails
	{
		void CopyFromStateDetails(const SAnimNodeDetails& pNodeDetails);
		void CopyToStateDetails(SAnimNodeDetails& pNodeDetails);

		CButton m_fullRootPriority;
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
		CFillSliderCtrl	m_stayInStateTimer;
		CEdit m_transitionTime;
		CEdit m_keyTime;
	};

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

	CComboBox m_presetList;
	CEdit m_stateNameEditBox;
	CEdit m_animationNameEditBox;
	CEdit m_layer;

	SDisplayDetails m_displayDetails;
	SGraphDisplayDetails m_graphDisplayDetails;

	CButton m_UnHideParameters;
	bool		m_ParametersAreHidden;
};


//////////////////////////////////////////////////////////////////////////
//     Movement Control and Collider Mode Panel
//////////////////////////////////////////////////////////////////////////

class CAnimationGraphAnimationNodeDetails_MCMColl : public CDialog
{
public:
	CAnimationGraphAnimationNodeDetails_MCMColl();
	virtual ~CAnimationGraphAnimationNodeDetails_MCMColl() {};
	void DoDataExchange(CDataExchange* pDX);
	void Init();
	void SetActiveAnimNode( CAGState2Ptr animNode );

	//! This will gray out all the data on the pane that cannot be changed 
	//! on a per-parameterization basis (which at this point means everything gray)
	void OnChangedParameterization();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnMCMChanged();
	afx_msg void OnAngleOrDistanceChanged();
	afx_msg void OnColliderModeSelected();

private:

	//! Checks if horizontal or vertical mcm is decoupled and if neither is
	//! it grays out the edit boxes for the angle and position deviation
	void GrayOutIfNotDecoupled();

	//! Called to check whether any graph default decoupled methods are used, and if so
	//! copies the values from there into the state and displays them
	//! on the panel
	void UpdateAngleAndPositionOverride();

	struct SMCMCollDetails
	{
		//void CopyFromStateDetails(const SGraphNodeDetails& pNodeDetails);
		//void CopyToStateDetails(SGraphNodeDetails& pNodeDetails) const;

		CComboBox	m_colliderMode;

		CComboBox	m_verticalMCM;
		CComboBox	m_horizontalMCM;

		CEdit	m_angle;
		CEdit	m_distance;
	};

	SMCMCollDetails	m_panelItems;
	CAGState2Ptr		m_pAnimNode;
};


#endif
