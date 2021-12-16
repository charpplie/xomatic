////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2014.
// -------------------------------------------------------------------------
//  File name:   WorldDataStatusPanel.h
//  Version:     v1.00
//  Created:     27/4/2014 by Allen Chen
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __WORLD_DATA_STATUS_PANEL_H__
#define __WORLD_DATA_STATUS_PANEL_H__

#include "SLDataPanel.h"
#include "SWCommon.h"

class CWorldDataStatusEntry : public CSLBaseItemRecord
{
public:
	CWorldDataStatusEntry(sw::EWDBType eType);

	void CreateItems();
	uint32 GetFileSCMAttributes();
	void GetFilename(std::vector<CString> &filenames) const { return filenames.push_back(m_filename); }

protected:
	sw::EWDBType m_eType;
	CString m_filename;
};

class CWorldDataStatusPanel : public CSLDataPanel
{
public:
	CWorldDataStatusPanel(CWnd *pParent = NULL);

	enum { IDD = IDD_PANEL_SEGMENT_STATUS };

	void UpdateEntries();

protected:
	virtual BOOL OnInitDialog();
};

#endif // __WORLD_DATA_STATUS_PANEL_H__