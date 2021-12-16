////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:  MotionAdaptorDlg.h
//  Version:    v1.00
//  Created:    16/4/2009 by Xiaomao Wu.
//  Compiler:   Visual Studio 2005 Professional
//  Description:

// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MOTIONADAPTORDLG_H__
#define __MOTIONADAPTORDLG_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h"
#include "..\ToolbarDialog.h"
#include "DockingPane/XTPDockingPane.h"
#include "..\UIPanels/TaskPanelDlg.h"
#include "..\CharacterEditor\MotionAdaptorManager.h"

// CMotionAdaptorDlg dialog

class CMotionAdaptorDlg : public CToolbarDialog
{
private:
	class CXTPTaskPanelSpecific: public CXTPTaskPanel
	{
	public:
		CXTPTaskPanelSpecific():CXTPTaskPanel(){};
		~CXTPTaskPanelSpecific(){};

		BOOL OnCommand(WPARAM wParam,LPARAM lParam);

		CMotionAdaptorDlg* GetOwner() const { return m_poMyOwner; }
		void SetOwner(CMotionAdaptorDlg* val) { m_poMyOwner = val; }
		
		LRESULT WindowProc(UINT message,WPARAM wParam,LPARAM lParam);
	protected:
		CMotionAdaptorDlg* m_poMyOwner;
	};

	DECLARE_DYNAMIC(CMotionAdaptorDlg)

public:
	CMotionAdaptorDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMotionAdaptorDlg();

	// Dialog Data
	enum { IDD = IDD_MOTION_ADAPTOR_DIALOG };

	CMotionAdaptorManager          m_dlgMain;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnTaskPanelNotify( WPARAM wParam, LPARAM lParam );
	afx_msg void OnReportSelChange( NMHDR * pNotifyStruct, LRESULT *result );
	void OnReportItemDblClick();
	void OnBnClickedFixInverseBending();
	void OnBnClickedMotionRetargeting();
	void OnBnClickedAdditiveMotion();
	void OnBnClickedAdjustFoot();
	void OnBnClickedShowViconMarkers();
	void onBnClickedShowViconSkeleton();


	CModelViewportCE* m_pModelViewportCE;
	CCharacterEditor* m_pCharacterEditor;
	CXTPDockingPane* m_pParentPane;

	void SetProgressEditText(const char* txt);
	void SetProgressSliderPos(const int32 pos);
	void SetModelViewportCE (CModelViewportCE* pMCE){m_pModelViewportCE = pMCE;}
	CModelViewportCE* GetModelViewportCE() {return m_pModelViewportCE;}
	void SetCharacterEditor(CCharacterEditor* pCE){m_pCharacterEditor = pCE;}
	CCharacterEditor* GetCharacterEditor() {return m_pCharacterEditor;}

	void SetPlayPauseText(const char* txt) {m_PlayPause->SetCaption(txt);}
	void SetMotionNameText(const char* txt) {m_MotionName->SetCaption(txt);}
	void SetViconFrameRateText(const char* txt) {m_ViconFrameRate->SetCaption(txt);}
	void SetBipedFrameRateText(const char* txt) {m_BipedFrameRate->SetCaption(txt);}
	void SetBipedSpeedText(const char* txt) {m_BipedSpeed->SetCaption(txt);}
	void SetBatchConvertInfoText(const char* txt) {m_CurrBatchConvertName->SetCaption(txt);}

protected:
	CXTPTaskPanelSpecific         m_wndTaskPanel;

	std::vector<CMotionAdaptorDlg*> m_aPartEditPanes;
private:
	CSliderCtrl m_AnimProgressSlider;
	CEdit m_AnimProgressEdit;
	CButton m_FixInverseBending;
	CButton m_MotionRetargeting;
	CButton m_ShowViconMarkers;
	CButton m_ShowViconSkeleton;
	CButton m_AdditiveMotion;
	CButton m_AdjustFoot;
	CXTPTaskPanelGroupItem* m_CurrBatchConvertName;
	CXTPTaskPanelGroupItem* m_MotionName;
	CXTPTaskPanelGroupItem* m_ViconFrameRate;
	CXTPTaskPanelGroupItem* m_BipedFrameRate;
	CXTPTaskPanelGroupItem* m_BipedSpeed;
	CXTPTaskPanelGroupItem* m_PlayPause;
	CXTPReportControl m_wndReport;
	std::vector<string> m_VFileNames;

private:
	void UpdateVFileList();
	void GetSelectedVFiles(std::vector<string>& vFilePaths);

};

#endif