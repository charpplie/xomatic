////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   LMGEditor.h
//  Version:     v1.00
//  Created:     07/07/2009 by Pau Novau
//  Description: Locomotion Group Editor main controller.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __LMGEditor__h__
#define __LMGEditor__h__
#pragma once

#include "Dialogs/BaseFrameWnd.h"

#include "LMGEditor/PreviewLMGView.h"
#include "LMGEditor/AnimationsListView.h"
#include "LMGEditor/SlotsView.h"
#include "LMGEditor/TestView.h"
#include "LMGEditor/BlendCodeSelectionView.h"
#include "LMGEditor/CapsCodeSelectionView.h"
#include "LMGEditor/MotionCombinationSlotsView.h"

#include "LMGEditor/ModelViewportLMG.h"

#include "LMGEditor/IAnimationNameSelectionChangedListener.h"

struct ICharacterInstance;
struct IAnimationSet;

#include "LMGInfo.h"

struct IUiMotionParamMapping;

class CLMGEditor
	: public CBaseFrameWnd
	, public IModelViewportLMGListener
	, public IAnimationNameSelectionChangedListener
	, public IBlendCodeChangedListener
	, public ICapsCodeChangedListener
	, public IMotionCombinationSlotsViewListener
	, public ISlotsViewListener
{
	DECLARE_DYNCREATE( CLMGEditor )

public:
	CLMGEditor();
	virtual ~CLMGEditor();


	void SetCapsCode( const CString& capsCode );
	void SetBlendCode( const CString& code );
	void TestStuff();

	// IModelViewportLMGListener
	virtual void PreDrawCharacter( ICharacterInstance* pCharacter, f32 frameTime );
	// ~IModelViewportLMGListener

	// IAnimationNameSelectionChangedListener
	virtual void AnimationNameSelectionChanged( const CString& currentSelectedAnimationName );
	// ~IAnimationNameSelectionChangedListener

	// IBlendCodeChangedListener
	virtual void OnBlendCodeChanged( const CString& blendCode );
	// ~IBlendCodeChangedListener

	// ICapsCodeChangedListener
	virtual void OnCapsCodeChanged( const CString& capsCode );
	// ~ICapsCodeChangedListener

	// IMotionCombinationSlotsViewListener
	virtual void OnMotionCombinationSlotNameChanged();
	virtual void OnMotionCombinationSlotRemoved();
	// ~IMotionCombinationSlotsViewListener

	// ISlotsViewListener
	virtual void OnSlotAnimationNameChanged();
	// ~ISlotsViewListener

protected:
	virtual BOOL OnInitDialog();

	CXTPDockingPane* AttachDockingWnd( UINT wndId, CWnd* pWnd, const CString& dockingPaneTitle, XTPDockingPaneDirection direction = xtpPaneDockLeft, CXTPDockingPaneBase* pNeighbour = NULL );

	afx_msg void OnUndo();
	afx_msg void OnRedo();
	afx_msg void OnFileLoadLmg();
	afx_msg void OnFileSaveLmg();
	afx_msg void OnFileLoadCharacter();
	afx_msg void OnOverlayParametersValues();
	afx_msg void OnUpdateOverlayParametersValues( CCmdUI* pCmdUI );
	afx_msg void OnOverlayAnimationWeights();
	afx_msg void OnUpdateOverlayAnimationWeights( CCmdUI* pCmdUI );
	DECLARE_MESSAGE_MAP()

	void LoadCharacter( const CString& filename );

	bool ValidateSlots();

	CString GenerateLmgXml();

	void FillSlotsView( const CString& code );
	void FillCapsCodeView();
	void FillTestView();

	void ClearHeldSlotValues();
	void HoldCurrentSlotValues();
	void FetchHeldSlotValues();

private:
	CPreviewLMGView m_previewLmgView;
	CPreviewLMGView m_previewAnimationView;
	CAnimationsListView m_animationListView;
	CSlotsView m_slotsView;
	CTestView m_testView;
	CBlendCodeSelectionView m_blendCodeSelectionView;
	CCapsCodeSelectionView m_capsCodeSelectionView;
	CMotionCombinationSlotsView m_motionCombinationSlots;

	ICharacterInstance* m_pCharacterInstance;
	IAnimationSet* m_pAnimationSet;

	CLMGInfo m_lmgInfo;
	CLMGInfo_BlendType* m_pSelectedBlendType;
	CLMGInfo_CapsCode* m_pSelectedCapsCode;

	CUiMotionParamMappings m_uiParamMappings;

	std::map< CString, CString > m_heldSlotIdToAnimationNames;

	bool m_overlayParameterValues;
	bool m_overlayAnimationWeights;
};

#endif