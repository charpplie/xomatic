// (c) 2001-2012 Crytek GmbH
// 8 Aug 2012: Created by Sergiy Shaykin
// Panel for controlling CSplineObject objects

#pragma once

#include "Controls/ToolButton.h"

class CSplineObject;



class CSplineEditButton : public CToolButton
{
protected:
	afx_msg void OnClicked();
	DECLARE_MESSAGE_MAP()
};



class CSplinePanel : public CDialog
{
	DECLARE_DYNAMIC(CSplinePanel)

public:
	CSplinePanel(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSplinePanel();

// Dialog Data
	enum { IDD = IDD_PANEL_SPLINE };

	void SetSpline(CSplineObject* pSpline);
	CSplineObject* GetSpline() const { return m_pSpline; }

	void Update();
	void OnUpdateParams( CNumberCtrl *ctrl );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedSelect();
	afx_msg void OnAlignHeightMap();
	afx_msg void OnDefaultWidth();

	virtual void OnOK() {};
	virtual void OnCancel() {};

	DECLARE_MESSAGE_MAP()

	CSplineObject* m_pSpline;
	CCustomButton m_alignHmapButton;
	CSplineEditButton m_editSplineButton;
	CToolButton m_splitButton;
	CToolButton m_mergeButton;

	CNumberCtrl m_angle;
	CNumberCtrl m_width;
};
