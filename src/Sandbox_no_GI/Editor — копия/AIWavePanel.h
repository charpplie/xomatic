
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   WavePanel.h
//  Version:     v1.00
//  Created:     2.09.2002 by Evgeny.
//  Compilers:   Visual C++.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __aiwavepanel_h__
#define __aiwavepanel_h__

#pragma once

#include "EntityPanel.h"
#include "Resource.h"


//////////////////////////////////////////////////////////////////////////
class CAIWavePanel : public CEntityPanel
{
	DECLARE_DYNAMIC(CAIWavePanel)

public:
	CAIWavePanel(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	enum { IDD = IDD_PANEL_AITERRITORY };	// NB: The same panel as for AITerritories

	void UpdateAssignedAIsPanel();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	
	afx_msg void OnSelectAssignedAIs();

	DECLARE_MESSAGE_MAP()
	
	CCustomButton	m_SelectAssignedAIsButton;
};

#endif // __aiwavepanel_h__