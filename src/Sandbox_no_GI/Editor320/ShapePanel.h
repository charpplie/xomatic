
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   shapepanel.h
//  Version:     v1.00
//  Created:     28/2/2002 by Timur.
//  Compilers:   Visual C++.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __shapepanel_h__
#define __shapepanel_h__
#pragma once

#include "Controls\PickObjectButton.h"
#include "Controls\ToolButton.h"

#include "Resource.h"

#include "EntityPanel.h"

class CAITerritoryObject;
class CShapeObject;

// CShapePanel dialog
class CShapePanel : public CDialog, public IPickObjectCallback
{
	DECLARE_DYNAMIC(CShapePanel)

public:
	CShapePanel( CWnd* pParent = NULL );   // standard constructor
	virtual ~CShapePanel();

// Dialog Data
	enum { IDD = IDD_PANEL_SHAPE };

	void SetShape( CShapeObject *shape );
	CShapeObject* GetShape() const { return m_shape; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedSelect();
	afx_msg void OnBnClickedRemove();
	afx_msg void OnBnClickedReverse();
	afx_msg void OnBnClickedReset();
	afx_msg void OnBnClickedUseTransformGizmo();
	afx_msg void OnLbnDblclkEntities();
	afx_msg void OnDestroy();

	virtual void OnOK() {};
	virtual void OnCancel() {};

	// Ovverriden from IPickObjectCallback
	virtual void OnPick( CBaseObject *picked );
	virtual bool OnPickFilter( CBaseObject *picked );
	virtual void OnCancelPick();

	DECLARE_MESSAGE_MAP()

	void ReloadEntities();

	bool	m_useTransforGizmo;

	CShapeObject *m_shape;
	CPickObjectButton m_pickButton;
	CCustomButton m_removeButton;
	CCustomButton m_selectButton;
	CToolButton m_editShapeButton;
	CToolButton m_splitShapeButton;
	CColoredListBox m_entities;
	CCustomButton m_reverseButton;
	CCustomButton m_resetButton;
};



//////////////////////////////////////////////////////////////////////////////
//
// CShapeMultySelPanel dialog
//////////////////////////////////////////////////////////////////////////////
class CShapeMultySelPanel : public CDialog
{
	DECLARE_DYNAMIC(CShapeMultySelPanel)

public:
	CShapeMultySelPanel( CWnd* pParent = NULL);   // standard constructor
	virtual ~CShapeMultySelPanel();

// Dialog Data
	enum { IDD = IDD_PANEL_SHAPE_MULTYSEL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	virtual void OnOK() {};
	virtual void OnCancel() {};

	DECLARE_MESSAGE_MAP()

	CToolButton m_mergeButton;
};


//////////////////////////////////////////////////////////////////////////
class CRopePanel : public CShapePanel
{
public:
	enum { IDD = IDD_PANEL_ROPE };

	CRopePanel( CWnd* pParent = NULL);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
};


//////////////////////////////////////////////////////////////////////////
class CAITerritoryPanel : public CEntityPanel
{
	DECLARE_DYNAMIC(CAITerritoryPanel)

public:
	CAITerritoryPanel(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	enum { IDD = IDD_PANEL_AITERRITORY };

	void UpdateAssignedAIsPanel();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	
	afx_msg void OnSelectAssignedAIs();

	DECLARE_MESSAGE_MAP()
	
	CCustomButton	m_SelectAssignedAIsButton;
};

#endif // __shapepanel_h__